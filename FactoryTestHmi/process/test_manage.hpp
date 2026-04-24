//
// Created by LEGION on 2026/4/17.
//

#ifndef FACTORYTESTMODULE_TEST_MANAGE_H
#define FACTORYTESTMODULE_TEST_MANAGE_H
#include "factory_protocol.hpp"
#include "test_config.hpp"
#include "qserial_channel.hpp"
class TestManage {
    public:
    enum TestState {
        Disconnect,
        Standby,
        Busy
    };
    TestManage();
    ~TestManage();

    bool connect() {
        if (m_state != Disconnect) {
            return true;
        }

        m_working = false;
        if (m_worker.joinable()) {
            m_worker.join();
        }

        if (m_connect_protocol) {
            m_connect_protocol->reset();
            delete m_connect_protocol;
        }

        auto cfg = Config::instance();
        m_connect_protocol = new CFactoryTestProtocol(new SerialChannel(cfg.m_connect_serial.com, static_cast<QSerialPort::BaudRate>(cfg.m_connect_serial.baud_rate)));
        m_connect_protocol->start();

        m_working = true;
        m_worker = std::thread([this]() {
            while (m_working) {
                handleMsg();
            }
        });
    }
    void disconnect() {

    }

    void handleMsg() {
        auto msg = m_connect_protocol->pull();
        if (m_state == Disconnect || m_state == Standby) {
            return;
        }
        qDebug() << "recv msg = " << CUtils::formatHexToStr(reinterpret_cast<const uint8_t *>(msg.data), msg.data_len);
        CodeEntity* code_entity = CFactoryTestProtocol::parse_code_entity();
    }


private:
    CFactoryTestProtocol* m_connect_protocol = nullptr;
    std::thread m_worker;
    TestState m_state = Disconnect;
    bool m_working = false;
};

#endif //FACTORYTESTMODULE_TEST_MANAGE_H
