//
// Created by LEGION on 2026/4/17.
//

#ifndef FACTORYTESTMODULE_TEST_MANAGE_H
#define FACTORYTESTMODULE_TEST_MANAGE_H

#include <QObject>
#include <atomic>
#include <thread>
#include <mutex>
#include <QDebug>

#include "factory_protocol.hpp"
#include "test_config.hpp"
#include "qserial_channel.hpp"
#include "test_rt_model.hpp"
#include "common.hpp"
#include "test_controller.hpp"


// ---------- 功能码 ----------
namespace MsgFuncCode {
    constexpr uint8_t HANDSHAKE        = 0x01;   // 设备 → 上位机
    constexpr uint8_t TEST_RESULT      = 0x02;   // 设备 → 上位机
    constexpr uint8_t DEVICE_STATUS    = 0x03;   // 设备 → 上位机
    constexpr uint8_t CMD_START_TEST   = 0x80;   // 上位机 → 设备（保留）
    constexpr uint8_t CMD_CFG_PARAM    = 0x81;   // 上位机 → 设备
    constexpr uint8_t CMD_TEST_LIST    = 0x82;   // 上位机 → 设备
}

// ---------- 测试管理类（状态机 + 线程 + 协议适配）----------
class TestManage : public QObject {
    Q_OBJECT
    DECLARE_SINGLETON(TestManage)

public:
    enum TestState {
        Disconnect,
        Standby,
        Busy            // 测试进行中（包括等待握手、等待测试结果）
    };
    Q_ENUM(TestState)

    Q_INVOKABLE bool connect() {
        if (m_state != Disconnect) {
            return false;
        }


        m_working = false;
        if (m_worker.joinable()) m_worker.join();

        if (m_connect_protocol) {
            m_connect_protocol->reset();
            delete m_connect_protocol;
            m_connect_protocol = nullptr;
        }

        auto cfg = Config::instance();
        m_connect_protocol = new CFactoryTestProtocol(
            new SerialChannel(cfg->connectSerial().com,static_cast<QSerialPort::BaudRate>(cfg->connectSerial().baud_rate)));
        if (!m_connect_protocol->start()) {
            delete m_connect_protocol;
            m_connect_protocol = nullptr;
            return false;
        }

        m_working = true;
        m_worker = std::thread([this]() {
            while (m_working) {
                // 接收消息
                auto msg = m_connect_protocol->pull();
                if (msg.data_len > 0) {
                    handleMsg(msg);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        });

        m_state = Standby;
        emit stateChanged(m_state);
        return true;
    }

    Q_INVOKABLE void disconnect() {
        if (m_state == Disconnect) return;
        if (m_state == Busy) stop();

        m_working = false;
        if (m_worker.joinable()) m_worker.join();

        if (m_connect_protocol) {
            m_connect_protocol->reset();
            delete m_connect_protocol;
            m_connect_protocol = nullptr;
        }

        m_state = Disconnect;
        emit stateChanged(m_state);
    }

    Q_INVOKABLE void start() {
        if (m_state != Standby) {
            return;
        }

        //QVector<TestItem> plan = Config::instance()->currentTestPlan();
        // QVector<TestRunItem> plan;
        // if (plan.isEmpty()) {
        //     qWarning() << "Test plan is empty";
        //     return;
        // }

        // m_flowController->loadTestPlan(plan);
        // m_flowController->startAll();          // 立即启动所有定时器！
        m_handshakeDone = false;               // 标记握手尚未发生
        m_state = Busy;
        emit stateChanged(m_state);
    }

    Q_INVOKABLE void restart() {
        if (m_state != Busy) return;
        m_flowController->restartAll();
        m_handshakeDone = false;               // 重新测试需要重新握手
        // 状态仍为 Busy
        emit stateChanged(m_state);
    }

    Q_INVOKABLE void stop() {
        if (m_state != Busy) return;
        m_flowController->stopAll();
        m_state = Standby;
        emit stateChanged(m_state);
    }

    Q_INVOKABLE void reset() {
        if (m_state == Busy) stop();
        m_flowController->resetAll();
        m_state = Standby;
        emit stateChanged(m_state);
    }

signals:
    void stateChanged(int state);
    void responseReceived(int configId);   // 内部跨线程

private:
    TestManage() {
        m_rt = RtModel::instance();
        m_flowController = new TestFlowController(m_rt, this);

        // 跨线程：工作线程 → 主线程
        QObject::connect(this, &TestManage::responseReceived, m_flowController, &TestFlowController::onResponseReceived,Qt::QueuedConnection);

        // 所有测试项完成，自动切回 Standby
        QObject::connect(m_flowController, &TestFlowController::allItemsFinished, this, [this]() {
            if (m_state == Busy) {
                m_state = Standby;
                emit stateChanged(m_state);
                qDebug() << "所有测试项已完成，返回 Standby";
            }
        });

        m_state = Disconnect;
        this->connect();
        start();
    }

    ~TestManage() {
        disconnect();
    }

    // 工作线程中的消息处理（状态机）
    void handleMsg(const MessageEntity &msg) {
        if (msg.data_len < 1 || msg.data.empty()) return;
        uint8_t func = msg.data[0];

        switch (m_state.load()) {
        case Disconnect:
            break;
        case Standby:
            handleStandbyMsg(msg, func);
            break;
        case Busy:
            handleBusyMsg(msg, func);
            break;
        }
    }

    void handleStandbyMsg(const MessageEntity &msg, uint8_t func) {
        Q_UNUSED(msg); Q_UNUSED(func);
        // 处理设备状态上报等
    }

    void handleBusyMsg(const MessageEntity &msg, uint8_t func) {
        switch (func) {
        case MsgFuncCode::HANDSHAKE:
            if (!m_handshakeDone) {
                m_handshakeDone = true;
                qDebug() << "收到握手，下发配置和测试项列表";

                // 构造并发送测试参数配置
                MessageEntity cfgMsg;
                cfgMsg.index = 0;
                cfgMsg.type  = 0;
                cfgMsg.data  = { static_cast<uint8_t>(MsgFuncCode::CMD_CFG_PARAM), 0 };
                cfgMsg.data_len = static_cast<short>(cfgMsg.data.size());
                m_connect_protocol->push(cfgMsg);

                // 构造并发送测试项列表
                MessageEntity listMsg;
                listMsg.index = 0;
                listMsg.type  = 0;
                listMsg.data  = { static_cast<uint8_t>(MsgFuncCode::CMD_TEST_LIST), 0 };
                listMsg.data_len = static_cast<short>(listMsg.data.size());
                m_connect_protocol->push(listMsg);
            }
            break;

        case MsgFuncCode::TEST_RESULT:
            // 假设 msg.index 就是测试项 id
            emit responseReceived(msg.index);
            break;

        case MsgFuncCode::DEVICE_STATUS:
            qDebug() << "设备状态上报";
            break;

        default:
            break;
        }
    }

    std::atomic<TestState> m_state{Disconnect};
    RtModel* m_rt = nullptr;
    TestFlowController* m_flowController = nullptr;
    CFactoryTestProtocol* m_connect_protocol = nullptr;

    std::thread m_worker;
    std::atomic<bool> m_working{false};

    bool m_handshakeDone = false;   // 是否已发送过配置和列表
};

#endif // FACTORYTESTMODULE_TEST_MANAGE_H
