#include "comm_debug_manage.hpp"
#include "comm_debug_receiver.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <chrono>
#include <cstring>
#include <thread>

namespace {
QString hardcodedJsonPath()
{
    return QCoreApplication::applicationDirPath()
        + QStringLiteral("/config/handshake.json");
}
} // namespace

CommDebugManage::CommDebugManage(QObject *parent)
    : QObject(parent)
{
    m_receiver = new CommDebugReceiver(this);
    m_receiver->setTxMutex(&m_serialTxMutex);
    connect(m_receiver, &CommDebugReceiver::statusMessage, this, &CommDebugManage::recvStatus,
            Qt::QueuedConnection);
    connect(m_receiver, &CommDebugReceiver::jsonReceived, this, &CommDebugManage::recvJsonLog,
            Qt::QueuedConnection);
    // 必须 Direct：sendJson 会在调用线程 wait ACK；若 Queued 到被堵住的 UI 线程会死锁 → 假超时
    connect(m_receiver, &CommDebugReceiver::ackReplied, this, [this](int generation) {
        onAckDone(generation);
    }, Qt::DirectConnection);
    connect(m_receiver, &CommDebugReceiver::ackReplied, this, [this](int) {
        ++m_ackCount;
        emit ackCountChanged();
    }, Qt::QueuedConnection);
    connect(m_receiver, &CommDebugReceiver::statusMessage, this, [this](const QString &) {
        emit serverListeningChanged();
    }, Qt::QueuedConnection);
}

CommDebugManage::~CommDebugManage()
{
    if (m_receiver)
        m_receiver->stop();
    m_ackCv.notify_all();
}

void CommDebugManage::takeOver(CFactoryTestProtocol *ch)
{
    if (m_receiver && m_receiver->isRunning())
        m_receiver->stop();

    m_uart_ch = ch;
    if (m_receiver)
        m_receiver->setChannel(m_uart_ch);
    if (m_uart_ch) {
        m_uart_ch->clearQueue();
        m_uart_ch->clearRxBuffer();
    }
    m_inFlight.store(false);
    emit sendStatus(QStringLiteral("[OK] 通讯调试已接管串口（物理 TX/RX：客户端 push，服务器 pull）"));
    emit serverListeningChanged();
}

void CommDebugManage::release()
{
    if (m_receiver)
        m_receiver->stop();
    if (m_uart_ch) {
        m_uart_ch->clearQueue();
        m_uart_ch->clearRxBuffer();
    }
    m_uart_ch = nullptr;
    if (m_receiver)
        m_receiver->setChannel(nullptr);
    m_inFlight.store(false);
    m_ackCv.notify_all();
    emit serverListeningChanged();
}

QString CommDebugManage::jsonPath() const
{
    return hardcodedJsonPath();
}

bool CommDebugManage::serverListening() const
{
    return m_receiver && m_receiver->isRunning();
}

int CommDebugManage::sendCount() const
{
    return m_sendCount;
}

int CommDebugManage::ackCount() const
{
    return m_ackCount;
}

void CommDebugManage::resetCounts()
{
    if (m_sendCount != 0) {
        m_sendCount = 0;
        emit sendCountChanged();
    }
    if (m_ackCount != 0) {
        m_ackCount = 0;
        emit ackCountChanged();
    }
}

void CommDebugManage::onAckDone(int generation)
{
    {
        std::lock_guard<std::mutex> lock(m_ackWaitMutex);
        if (generation >= m_ackedGeneration.load())
            m_ackedGeneration.store(generation);
        m_inFlight.store(false);
    }
    m_ackCv.notify_all();
}

bool CommDebugManage::waitForAck(int generation, int timeoutMs)
{
    std::unique_lock<std::mutex> lock(m_ackWaitMutex);
    return m_ackCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, generation]() {
        return m_ackedGeneration.load() >= generation || !m_inFlight.load();
    }) && m_ackedGeneration.load() >= generation;
}

bool CommDebugManage::pushRequestOnce(const QByteArray &payload, int generation)
{
    MessageEntity msg{};
    msg.index = 0;
    msg.type = 0x02;
    msg.data = strdup(payload.constData());
    msg.data_len = static_cast<short>(payload.size());

    if (m_receiver)
        m_receiver->prepareForNewRequest(generation);

    {
        std::lock_guard<std::mutex> lock(m_serialTxMutex);
        // 只清 RX 字节残留，不清消息队列——避免把「已解出、尚未 pull」的请求丢掉
        m_uart_ch->clearRxBuffer();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        m_uart_ch->push(msg);
        // 等环回进队列并完成解帧（写路径已边写边吸 RX）
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return true;
}

QString CommDebugManage::toPrettyJson(const QByteArray &raw)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || doc.isNull())
        return QString::fromUtf8(raw);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

void CommDebugManage::sendJson()
{
    if (!m_uart_ch) {
        const QString err = QStringLiteral("[ERR] 串口通道未接管，请先切换到「通讯调试」");
        emit sendStatus(err);
        emit sendFinished(false, err);
        return;
    }

    if (m_inFlight.load()) {
        const QString err = QStringLiteral("[WARN] 上一次发送仍在等待回传，请稍候再发（避免叠帧）");
        emit sendStatus(err);
        emit sendFinished(false, err);
        return;
    }

    const QString path = hardcodedJsonPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        const QString err = QStringLiteral("[ERR] 无法打开 JSON: %1").arg(path);
        emit sendStatus(err);
        emit sendFinished(false, err);
        return;
    }

    const QByteArray payload = file.readAll();
    if (payload.isEmpty()) {
        const QString err = QStringLiteral("[ERR] JSON 文件为空: %1").arg(path);
        emit sendStatus(err);
        emit sendFinished(false, err);
        return;
    }

    emit sendJsonLog(toPrettyJson(payload));
    emit sendStatus(QStringLiteral("[OK] 客户端发送 %1 (%2 字节)").arg(path).arg(payload.size()));

    const int generation = m_txGeneration.fetch_add(1) + 1;
    m_inFlight.store(true);

    ++m_sendCount;
    emit sendCountChanged();

    pushRequestOnce(payload, generation);
    emit sendStatus(QStringLiteral("[OK] 请求已经物理串口 TX 写出（第 %1 次 gen=%2）")
                        .arg(m_sendCount).arg(generation));

    if (!serverListening()) {
        m_inFlight.store(false);
        emit recvStatus(QStringLiteral("[WARN] 服务器未启动：请先点右侧「启动」"));
        emit sendFinished(false, QStringLiteral("服务器未启动"));
        return;
    }

    emit sendStatus(QStringLiteral("[OK] 等待服务器从串口 RX 解帧并回 ACK…"));

    // 首发等待；失败则物理重发 1 次（不增加发送计数）
    if (!waitForAck(generation, 900)) {
        emit sendStatus(QStringLiteral("[WARN] 首发未回传，物理重发 1 次（第 %1 次）").arg(m_sendCount));
        m_inFlight.store(true);
        pushRequestOnce(payload, generation);
        if (!waitForAck(generation, 900)) {
            m_inFlight.store(false);
            const QString err = QStringLiteral("[ERR] 仍无回传（第 %1 次）。同口环回半双工仍可能丢帧，请查线缆/换虚拟串口对")
                                    .arg(m_sendCount);
            emit sendStatus(err);
            emit sendFinished(false, err);
            return;
        }
        emit sendStatus(QStringLiteral("[OK] 重发后已收到回传（第 %1 次）").arg(m_sendCount));
    } else {
        emit sendStatus(QStringLiteral("[OK] 已收到回传（第 %1 次）").arg(m_sendCount));
    }

    emit sendFinished(true, QStringLiteral("发送完成"));
}

void CommDebugManage::startServer()
{
    if (!m_receiver)
        return;
    m_receiver->setChannel(m_uart_ch);
    m_receiver->setTxMutex(&m_serialTxMutex);
    m_receiver->start();
    emit serverListeningChanged();
}

void CommDebugManage::stopServer()
{
    if (!m_receiver)
        return;
    m_receiver->stop();
    m_inFlight.store(false);
    m_ackCv.notify_all();
    emit serverListeningChanged();
}
