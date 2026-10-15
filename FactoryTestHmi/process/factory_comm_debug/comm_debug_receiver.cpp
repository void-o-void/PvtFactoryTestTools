#include "comm_debug_receiver.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <chrono>
#include <cstring>
#include <thread>

CommDebugReceiver::CommDebugReceiver(QObject *parent)
    : QObject(parent)
{
}

CommDebugReceiver::~CommDebugReceiver()
{
    stop();
}

void CommDebugReceiver::setChannel(CFactoryTestProtocol *ch)
{
    m_uart_ch = ch;
}

void CommDebugReceiver::setTxMutex(std::mutex *txMutex)
{
    m_txMutex = txMutex;
}

void CommDebugReceiver::prepareForNewRequest(int generation)
{
    m_currentGeneration.store(generation);
    // 新一轮允许再回一次；不清 ackedGeneration 以外的历史——用 generation 比较即可
}

bool CommDebugReceiver::start()
{
    if (m_running.load()) {
        emit statusMessage(QStringLiteral("[WARN] 服务器已在监听"));
        return true;
    }
    if (!m_uart_ch) {
        emit statusMessage(QStringLiteral("[ERR] 串口通道未接管，无法启动服务器"));
        return false;
    }

    m_stop.store(false);
    m_running.store(true);
    m_ackedGeneration.store(-1);
    m_uart_ch->clearQueue();
    m_uart_ch->clearRxBuffer();
    m_worker = std::thread([this]() { workerLoop(); });
    emit statusMessage(QStringLiteral("[OK] 服务器已启动：pull 监听串口请求，整理后 push 回传 ACK"));
    return true;
}

void CommDebugReceiver::stop()
{
    if (!m_running.load() && !m_worker.joinable())
        return;

    m_stop.store(true);
    if (m_uart_ch)
        m_uart_ch->wakeUpQueue();

    if (m_worker.joinable())
        m_worker.join();

    m_running.store(false);
    emit statusMessage(QStringLiteral("[OK] 服务器已停止"));
}

QString CommDebugReceiver::toPrettyJson(const QByteArray &raw)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || doc.isNull())
        return QString::fromUtf8(raw);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QByteArray CommDebugReceiver::buildAckBytes(const QByteArray &requestRaw, QString *err)
{
    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(requestRaw, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (err)
            *err = QStringLiteral("请求不是合法 JSON 对象: %1").arg(parseErr.errorString());
        return {};
    }

    const QJsonObject req = doc.object();
    // 已是 ACK（顶层有 state、无 data）则不再回，避免死循环
    if (req.contains(QStringLiteral("state")) && !req.contains(QStringLiteral("data"))) {
        if (err)
            *err = QStringLiteral("忽略 ACK 帧，不再回传");
        return {};
    }
    if (!req.contains(QStringLiteral("func")) && !req.contains(QStringLiteral("request_id"))) {
        if (err)
            *err = QStringLiteral("缺少 func/request_id，无法整理 ACK");
        return {};
    }

    QJsonObject ack;
    ack.insert(QStringLiteral("func"), req.value(QStringLiteral("func")));
    ack.insert(QStringLiteral("sn"), req.value(QStringLiteral("sn")));
    ack.insert(QStringLiteral("plan_id"), req.value(QStringLiteral("plan_id")));
    ack.insert(QStringLiteral("request_id"), req.value(QStringLiteral("request_id")));

    const int func = req.value(QStringLiteral("func")).toInt(-1);
    if (func == 0) {
        ack.insert(QStringLiteral("time"),
                   QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    }

    ack.insert(QStringLiteral("state"), true);

    return QJsonDocument(ack).toJson(QJsonDocument::Compact);
}

void CommDebugReceiver::processClientRequest(const QByteArray &raw, bool replyOnSerial)
{
    const QByteArray trimmed = raw.trimmed();
    if (trimmed.isEmpty()) {
        emit statusMessage(QStringLiteral("[ERR] 空请求"));
        return;
    }

    QJsonParseError peekErr{};
    const QJsonDocument peekDoc = QJsonDocument::fromJson(trimmed, &peekErr);
    if (peekErr.error == QJsonParseError::NoError && peekDoc.isObject()) {
        const QJsonObject o = peekDoc.object();
        if (o.contains(QStringLiteral("state")) && !o.contains(QStringLiteral("data"))) {
            emit statusMessage(QStringLiteral("[OK] 收到本机 ACK 环回（已忽略，不再回传）"));
            return;
        }
    }

    const int gen = m_currentGeneration.load();
    // 同一轮发送只回一次 ACK（防解帧重复），不按固定 request_id 误杀下一轮
    if (gen > 0 && m_ackedGeneration.load() == gen) {
        emit statusMessage(QStringLiteral("[WARN] 本轮重复帧已忽略 gen=%1（防双回）").arg(gen));
        return;
    }

    emit statusMessage(QStringLiteral("[OK] 服务器收到请求 %1 字节 gen=%2").arg(trimmed.size()).arg(gen));
    emit jsonReceived(QStringLiteral("—— 收到请求 ——\n") + toPrettyJson(trimmed));

    QString err;
    const QByteArray ackBytes = buildAckBytes(trimmed, &err);
    if (ackBytes.isEmpty()) {
        emit statusMessage(QStringLiteral("[WARN] %1").arg(err));
        return;
    }

    emit statusMessage(QStringLiteral("[OK] 服务器整理 ACK 并回传"));
    emit jsonReceived(QStringLiteral("—— 回传 ACK ——\n") + toPrettyJson(ackBytes));

    if (replyOnSerial && m_uart_ch) {
        MessageEntity msg{};
        msg.index = 0;
        msg.type = 0x02;
        msg.data = strdup(ackBytes.constData());
        msg.data_len = static_cast<short>(ackBytes.size());

        if (m_txMutex) {
            std::lock_guard<std::mutex> lock(*m_txMutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            m_uart_ch->push(msg);
            // 吸完 ACK 环回再放开，减少与下次发送撞车
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            m_uart_ch->push(msg);
        }

        m_ackedGeneration.store(gen);
        emit ackReplied(gen);
        emit statusMessage(QStringLiteral("[OK] ACK 已经串口写出 gen=%1").arg(gen));
    }
}

void CommDebugReceiver::workerLoop()
{
    while (!m_stop.load()) {
        if (!m_uart_ch)
            break;

        MessageEntity msg = m_uart_ch->pull();
        if (m_stop.load())
            break;

        if (msg.type == 7 || msg.type == 8)
            continue;
        if (!msg.data || msg.data_len < 1)
            continue;

        const QByteArray raw(msg.data, msg.data_len);
        processClientRequest(raw, true);
    }
    m_running.store(false);
}
