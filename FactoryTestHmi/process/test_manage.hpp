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
    constexpr uint8_t HANDSHAKE        = 0x64;   // 设备 → 上位机
    constexpr uint8_t TEST_RESULT      = 0x02;   // 设备 → 上位机
    constexpr uint8_t DEVICE_STATUS    = 0x03;   // 设备 → 上位机
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
        Busy
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

        m_state = Disconnect;
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
        if (m_state != Disconnect) {
            return;
        }

        QVector<TestRunItem> plan = Config::instance()->enabledTestPlan();
        m_flowController->loadTestPlan(plan);
        m_flowController->startAll(); // 立即启动所有定时器！

        m_state = Standby;
        emit stateChanged(m_state);
    }

    Q_INVOKABLE void restart() {
        if (m_state != Busy) return;
        m_flowController->restartAll();

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
    void handshakeDone();                  // 收到设备握手，通知 QML 切换按钮状态
    void testFinished(int failCount);      // 测试全部完成，failCount=0 表示全部通过
    void responseReceived(int configId);   // 内部跨线程

private:
    TestManage() {
        m_rt = RtModel::instance();
        m_flowController = new TestFlowController(m_rt, this);

        // 跨线程：工作线程 → 主线程
        QObject::connect(this, &TestManage::responseReceived, m_flowController, &TestFlowController::onResponseReceived,Qt::QueuedConnection);

        // 所有测试项完成，通知 QML 结果后切回 Standby
        QObject::connect(m_flowController, &TestFlowController::allItemsFinished, this, [this]() {
            if (m_state == Busy) {
                int fail = m_rt->fail_num().toInt();
                emit testFinished(fail);
                qDebug() << "所有测试项已完成" << (fail == 0 ? "PASS" : "FAIL");
                m_state = Standby;
                emit stateChanged(m_state);
            }
        });

        m_state = Disconnect;
        this->connect();
        // 不在此处 start()，由 QML 按钮点击时调用 start() 进入 Busy
    }

    ~TestManage() {
        disconnect();
    }

    void handleMsg(const MessageEntity &msg) {
        if ( msg.data_len < 1 || msg.data == nullptr || m_state.load() == Disconnect) {
            return;
        }
        std::string buf(msg.data, msg.data_len);
        CodeEntity* request_code = CFactoryTestProtocol::parseCodeEntity(buf);
        if (request_code == nullptr) {
            qDebug() << "handleMsg: parseCodeEntity error!";
            return;
        }

        switch (m_state.load()) {
            case Standby:
                if (request_code->code != MsgFuncCode::HANDSHAKE) {
                    qDebug() << "收到错误消息, 请复位重新测试";
                    return;
                }
                handleHandShake();
                break;
            case Busy:
                handleBusyMsg(request_code);
                break;
            default: break;
        }
    }

    void handleHandShake() {
        qDebug() << "Standby 状态收到设备握手";
        emit handshakeDone();
        sleep(5);
        emit testFinished(0);
    }

    void handleBusyMsg(CodeEntity* request) {
        switch (request->code) {
            case MsgFuncCode::TEST_RESULT:
                qDebug() << "测项结果上报";
                break;
            case MsgFuncCode::DEVICE_STATUS:
                qDebug() << "设备状态上报";
                break;
            default: break;
        }
    }

    std::atomic<TestState> m_state{Disconnect};
    RtModel* m_rt = nullptr;
    TestFlowController* m_flowController = nullptr;
    CFactoryTestProtocol* m_connect_protocol = nullptr;

    std::thread m_worker;
    std::atomic<bool> m_working{false};
};

#endif // FACTORYTESTMODULE_TEST_MANAGE_H
