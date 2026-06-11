#include "aging_test_manage.hpp"
#include "test_config.hpp"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <thread>
#include <chrono>
#include <memory>

AmingTestManage::AmingTestManage(QObject *parent) : QObject(parent) {}

AmingTestManage::~AmingTestManage() { reset(); }

void AmingTestManage::takeOver(CFactoryTestProtocol *ch) {
    m_uart_ch = ch;
    // 接管时清空队列残留消息（上次会话的尾巴 + 功能测试推的唤醒消息）
    if (m_uart_ch) m_uart_ch->clearQueue();
}

// ==================== start ====================
void AmingTestManage::start() {
    reset();
    m_paused = false;

    m_worker = std::thread([this]() {
        bool exit = false;

        // 再次清空，防止接管后到 start 之间进入队列的消息
        if (m_uart_ch) m_uart_ch->clearQueue();

        while (!exit && !pushHandshake(exit)) {
            if (exit) return;
            emit logMessage("[WARN] 握手超时，重试...");
        }
        if (exit) return;
        emit handshakeDone();

        while (!exit && !pushConfig(exit)) {
            if (exit) return;
            emit logMessage("[WARN] 配置超时，重试...");
        }
        if (exit) return;
        emit configDone();

        while (!exit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 暂停状态：只休眠，不下发查询
            if (m_paused) continue;

            pushStatusQuery();

            // 超时保护：2 秒内无响应自动重试（应对 CProtocol 解码丢帧）
            auto active = std::make_shared<std::atomic<bool>>(true);
            std::thread([this, active]() {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                if (*active) {
                    MessageEntity wake; wake.type = 7;
                    m_uart_ch->pushQueue(wake);
                }
            }).detach();

            auto msg = m_uart_ch->pull();
            *active = false;

            if (msg.type == 8) break;
            if (msg.type == 7) continue;
            if (msg.data_len > 3) handleStatus(msg);
        }
    });
}

void AmingTestManage::setPaused(bool pause) {
    m_paused = pause;
    if (m_uart_ch) {
        MessageEntity wake; wake.type = 7;
        m_uart_ch->pushQueue(wake);  // 唤醒可能阻塞在 pull() 的轮询循环
    }
}

void AmingTestManage::reset() {
    m_paused = false;  // 取消暂停，让 worker 走出死循环
    if (m_worker.joinable()) {
        if (m_uart_ch) {
            MessageEntity wake; wake.type = 8; m_uart_ch->pushQueue(wake);
        }
        m_worker.join();
    }
    // 队列清空由 takeOver() / start() 中的 clearQueue() 负责
}

// ==================== pushHandshake ====================
bool AmingTestManage::pushHandshake(bool &exit) {
    QJsonObject obj;
    obj["code"] = 100;

    QJsonObject paraObj;
    paraObj["action"] = 2;
    paraObj["state"]  = 1;

    // para 编码：paraObj → JSON字符串 → 字节数组（与 handleHandShake 一致）
    QJsonDocument paraDoc(paraObj);
    QByteArray paraBytes = paraDoc.toJson(QJsonDocument::Compact);
    QJsonArray paraArray;
    for (int i = 0; i < paraBytes.size(); ++i)
        paraArray.append(static_cast<int>(static_cast<unsigned char>(paraBytes.at(i))));
    obj["para"] = paraArray;

    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity msg;
    msg.index = 0; msg.type = 2;
    msg.data = strdup(bytes.constData());
    msg.data_len = bytes.size();
    m_uart_ch->push(msg);

    emit logMessage("[OK] 发送握手，等待响应(2s)...");

    auto active = std::make_shared<std::atomic<bool>>(true);
    std::thread([this, active]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if (*active) {
            MessageEntity wake; wake.type = 7;
            m_uart_ch->pushQueue(wake);
        }
    }).detach();

    auto resp = m_uart_ch->pull();
    *active = false;

    if (resp.type == 8) {
        exit = true;
        return false;
    }

    if (resp.type == 7 || resp.type != 2) {
        return false;
    }

    std::string buf(resp.data, resp.data_len);
    CodeEntity* ce = CFactoryTestProtocol::parseCodeEntity(buf);
    if (ce && ce->code == 100) { emit logMessage("[OK] 握手成功"); return true; }
    return false;
}

// ==================== pushConfig ====================
bool AmingTestManage::pushConfig(bool &exit) {
    QJsonObject obj;
    obj["code"] = 101;

    QJsonObject paraObj;
    paraObj["agingTestDurationMin"] = 10;
    paraObj["agingHeartBeatIntervalSec"]  = 5;
    paraObj["agingCPULoading"]  = 50;
    paraObj["agingGPULoading"]  = 50;
    paraObj["agingAPULoading"]  = 50;
    paraObj["agingWifiScanWaitSec"]  = 30;
    paraObj["agingBtScanWaitSec"]  = 30;
    paraObj["agingDramLoopSec"]  = 60;

    QJsonDocument paraDoc(paraObj);
    QByteArray paraBytes = paraDoc.toJson(QJsonDocument::Compact);
    QJsonArray paraArray;
    for (int i = 0; i < paraBytes.size(); ++i)
        paraArray.append(static_cast<int>(static_cast<unsigned char>(paraBytes.at(i))));
    obj["para"] = paraArray;

    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity msg;
    msg.index = 0; msg.type = 2;
    msg.data = strdup(bytes.constData());
    msg.data_len = bytes.size();
    m_uart_ch->push(msg);

    emit logMessage("[OK] 发送配置，等待响应(2s)...");

    auto active = std::make_shared<std::atomic<bool>>(true);
    std::thread([this, active]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if (*active) {
            MessageEntity wake; wake.type = 7;
            m_uart_ch->pushQueue(wake);
        }
    }).detach();

    auto resp = m_uart_ch->pull();
    *active = false;

    if (resp.type == 8) { exit = true; return false; }
    if (resp.type == 7) return false;

    std::string buf(resp.data, resp.data_len);
    CodeEntity* ce = CFactoryTestProtocol::parseCodeEntity(buf);
    if (ce && ce->code == 101) { emit logMessage("[OK] 配置成功"); return true; }
    return false;
}

// ==================== 轮询 ====================
void AmingTestManage::pushStatusQuery() {
    QJsonObject obj;
    obj["code"] = 300;

    QJsonObject paraObj;               // 空参数，保持格式一致
    paraObj["action"] = 2;
    paraObj["state"]  = 1;
    QJsonDocument paraDoc(paraObj);
    QByteArray paraBytes = paraDoc.toJson(QJsonDocument::Compact);
    QJsonArray paraArray;
    for (int i = 0; i < paraBytes.size(); ++i)
        paraArray.append(static_cast<int>(static_cast<unsigned char>(paraBytes.at(i))));
    obj["para"] = paraArray;

    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity msg;
    msg.index = 0; msg.type = 2;
    msg.data = strdup(bytes.constData());
    msg.data_len = bytes.size();
    m_uart_ch->push(msg);
}

void AmingTestManage::handleStatus(const MessageEntity& msg) {
    std::string buf(msg.data, msg.data_len);
    CodeEntity* ce = CFactoryTestProtocol::parseCodeEntity(buf);
    if (!ce) return;
    if (ce->code == 300 && ce->common) {
        emit logMessage(QString("[OK] 状态上报 state:%1 %2")
            .arg(ce->common->state)
            .arg(ce->common->msg ? ce->common->msg : ""));

        // common->msg 是 JSON 字符串，包含温度和负载数据
        if (ce->common->msg && ce->common->msg[0] != '\0') {
            QJsonDocument doc = QJsonDocument::fromJson(ce->common->msg);
            if (doc.isObject()) {
                QJsonObject o = doc.object();
                emit tempDataUpdated(o["apNtcTemp"].toDouble(),
                                     o["mdTemp"].toDouble(),
                                     o["pmicTemp"].toDouble(),
                                     o["socMaxTemp"].toDouble());
                emit dashboardDataUpdated(o["cpuLoading"].toDouble(),
                                          o["gpuLoading"].toDouble(),
                                          o["apuLoading"].toDouble(),
                                          o["dramFreq"].toDouble());
            }
        }
    }
}
