#ifndef AGING_TEST_MANAGE_HPP
#define AGING_TEST_MANAGE_HPP

#include <QObject>
#include <thread>

#include "common.hpp"
#include "factory_protocol.hpp"

class AmingTestManage : public QObject {
    Q_OBJECT
    DECLARE_SINGLETON(AmingTestManage)

public:
    void takeOver(CFactoryTestProtocol* ch);

    Q_INVOKABLE void start();
    Q_INVOKABLE void reset();

private:
    AmingTestManage(QObject* parent = nullptr);
    ~AmingTestManage() override;

    bool pushHandshake(bool &exit);
    bool pushConfig(bool &exit);

    void pushStatusQuery();
    void handleStatus(const MessageEntity& msg);

    CFactoryTestProtocol* m_uart_ch = nullptr;
    std::thread m_worker;
};

#endif
