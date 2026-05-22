#ifndef AGING_TEST_MANAGE_HPP
#define AGING_TEST_MANAGE_HPP

#include <QObject>
#include <thread>

#include "factory_protocol.hpp"

class AmingTestManage : public QObject {
    Q_OBJECT
    DECLARE_SINGLETON(AmingTestManage)

public:
    void takeOver(CFactoryTestProtocol* ch);

    Q_INVOKABLE void start();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void setPaused(bool pause);

signals:
    void logMessage(const QString &msg);
    void handshakeDone();
    void configDone();

private:
    AmingTestManage(QObject* parent = nullptr);
    ~AmingTestManage() override;

    bool pushHandshake(bool &exit);
    bool pushConfig(bool &exit);
    void pushStatusQuery();
    void handleStatus(const MessageEntity& msg);
    void drainQueue();

    CFactoryTestProtocol* m_uart_ch = nullptr;
    std::thread m_worker;
    std::atomic<bool> m_paused{false};
};

#endif
