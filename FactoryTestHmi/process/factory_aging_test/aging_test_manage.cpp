#include "aging_test_manage.hpp"
#include "test_config.hpp"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <thread>
#include <chrono>
#include <memory>

AmingTestManage::AmingTestManage(QObject *parent) : QObject(parent) {}

AmingTestManage::~AmingTestManage() {
    reset();
}

void AmingTestManage::takeOver(CFactoryTestProtocol *ch) { m_uart_ch = ch; }

// ==================== start ====================
void AmingTestManage::start() {
    reset();
    m_worker = std::thread([this]() {
        bool exit = false;

        // 握手：超时重试，reset 退出
        while (!exit && !pushHandshake(exit)) {
            if (exit) {
                return;
            }
            qDebug() << "[Aging] 握手超时，重试...";
        }

        // 下发配置：超时重试，reset 退出
        while (!exit && !pushConfig(exit)) {
            if (exit)  {
                return;
            }
            qDebug() << "[Aging] 配置超时，重试...";
        }

        // 轮询
        while (!exit) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            pushStatusQuery();
            auto msg = m_uart_ch->pull();
            if (msg.type == 8) break;
            if (msg.type == 7) continue;
            if (msg.data_len > 3) handleStatus(msg);
        }
    });
}

void AmingTestManage::reset() {
    if (m_worker.joinable()) {
        if (m_uart_ch) {
            MessageEntity wake; wake.type = 8; m_uart_ch->pushQueue(wake);
        }
        m_worker.join();
    }
}

// ==================== pushHandshake ====================
bool AmingTestManage::pushHandshake(bool &exit) {
    QJsonObject obj;
    obj["code"] = 200;
    QJsonObject paraObj;
    paraObj["type"] = "aging_start";
    paraObj["factory"] = Config::instance()->factoryConfig().factory_name;
    obj["para"] = paraObj;
    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity msg;
    msg.index = 0; msg.type = 0;
    msg.data = strdup(bytes.constData());
    msg.data_len = bytes.size();
    m_uart_ch->push(msg);

    qDebug() << "[Aging] 发送握手，等待响应(2s)...";

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
    if (ce && ce->code == 100) { qDebug() << "[Aging] 握手成功"; return true; }
    return false;
}

// ==================== pushConfig ====================
bool AmingTestManage::pushConfig(bool &exit) {
    QJsonObject obj;
    obj["code"] = 201;
    QJsonObject paraObj;
    paraObj["duration_hours"] = 2;
    paraObj["target_temp"]    = 85;
    paraObj["voltage"]        = "12V";
    obj["para"] = paraObj;
    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity msg;
    msg.index = 0; msg.type = 0;
    msg.data = strdup(bytes.constData());
    msg.data_len = bytes.size();
    m_uart_ch->push(msg);

    qDebug() << "[Aging] 发送配置，等待响应(2s)...";

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
    if (ce && ce->code == 101) { qDebug() << "[Aging] 配置成功"; return true; }
    return false;
}

// ==================== 轮询 ====================
void AmingTestManage::pushStatusQuery() {
    QJsonObject obj; obj["code"] = 203;
    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity msg;
    msg.index = 0; msg.type = 0;
    msg.data = strdup(bytes.constData());
    msg.data_len = bytes.size();
    m_uart_ch->push(msg);
}

void AmingTestManage::handleStatus(const MessageEntity& msg) {
    std::string buf(msg.data, msg.data_len);
    CodeEntity* ce = CFactoryTestProtocol::parseCodeEntity(buf);
    if (!ce) return;
    if (ce->code == 203 && ce->common) {
        qDebug() << "[Aging] 状态上报 state:" << ce->common->state
                 << (ce->common->msg ? ce->common->msg : "");
    }
}
