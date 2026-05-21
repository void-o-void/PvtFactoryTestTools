#ifndef AGING_TEST_MANAGE_HPP
#define AGING_TEST_MANAGE_HPP

#include <QObject>
#include <thread>

#include "factory_protocol.hpp"
#include "qserial_channel.hpp"

class AmingTestManage : public QObject {
    Q_OBJECT
    DECLARE_SINGLETON(AmingTestManage)

public:
    Q_INVOKABLE void start();
    Q_INVOKABLE void reset();

signals:
    void logMessage(const QString &msg);

private:
    AmingTestManage(QObject* parent = nullptr);
    ~AmingTestManage() override;

    void openSerial();

    bool pushHandshake(bool &exit);
    bool pushConfig(bool &exit);
    void pushStatusQuery();
    void handleStatus(const MessageEntity& msg);

    CFactoryTestProtocol* m_uart_ch = nullptr;
    std::thread m_worker;
};

#endif
