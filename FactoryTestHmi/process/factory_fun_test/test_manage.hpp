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
#include <QJsonObject>
#include <QJsonDocument>

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
        Idle,
        Standby,
        Busy
    };
    Q_ENUM(TestState)

    Q_INVOKABLE void start();
    Q_INVOKABLE void reset();

signals:
    void stateChanged(int state);
    void handshakeDone();                  // 收到设备握手，通知 QML 切换按钮状态
    void testFinished(int failCount);      // 测试全部完成，failCount=0 表示全部通过
    void startAllRequested();                                     // 跨线程通知 flowController 启动定时器
    void itemResultReceived(int code, int state, const QString &msg); // 跨线程携带测试结果

private:
    TestManage();
    ~TestManage();

    void openSerial();
    void handleMsg(const MessageEntity &msg);
    void handleHandShake();
    void handleBusyMsg(CodeEntity* request);
    void sendCodeMessage(int code, const QJsonObject& paraObj);

    std::atomic<TestState> m_state{Idle};
    RtModel* m_rt = nullptr;
    TestFlowController* m_flowController = nullptr;
    CFactoryTestProtocol* m_connect_protocol = nullptr;

    std::thread m_worker;
    std::atomic<bool> m_working{false};
};

#endif // FACTORYTESTMODULE_TEST_MANAGE_H
