#ifndef FACTORYTESTMODULE_COMM_DEBUG_MANAGE_H
#define FACTORYTESTMODULE_COMM_DEBUG_MANAGE_H

#include <QObject>
#include <QString>

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "common.hpp"
#include "factory_protocol.hpp"

class CommDebugReceiver;

/** 通讯调试：上边客户端发 JSON；右侧启动服务器；服务器整理 ACK 回传并显示在下半区 */
class CommDebugManage : public QObject {
    Q_OBJECT
    DECLARE_SINGLETON(CommDebugManage)

    Q_PROPERTY(QString jsonPath READ jsonPath CONSTANT)
    Q_PROPERTY(bool serverListening READ serverListening NOTIFY serverListeningChanged)
    Q_PROPERTY(int sendCount READ sendCount NOTIFY sendCountChanged)
    Q_PROPERTY(int ackCount READ ackCount NOTIFY ackCountChanged)

public:
    void takeOver(CFactoryTestProtocol *ch);
    void release();

    /** 客户端：读写死路径 JSON，仅经物理串口 TX 发送 */
    Q_INVOKABLE void sendJson();
    /** 启动/停止服务器：仅从物理串口 RX pull，整理 ACK 后再 TX 回传 */
    Q_INVOKABLE void startServer();
    Q_INVOKABLE void stopServer();
    /** 清零发送次数 / 回传次数 */
    Q_INVOKABLE void resetCounts();

    QString jsonPath() const;
    bool serverListening() const;
    int sendCount() const;
    int ackCount() const;

    std::mutex &serialTxMutex() { return m_serialTxMutex; }
    void onAckDone(int generation);

signals:
    void sendJsonLog(const QString &prettyJson);
    void recvJsonLog(const QString &prettyJson);
    void sendStatus(const QString &msg);
    void recvStatus(const QString &msg);
    void serverListeningChanged();
    void sendCountChanged();
    void ackCountChanged();
    void sendFinished(bool ok, const QString &msg);

private:
    explicit CommDebugManage(QObject *parent = nullptr);
    ~CommDebugManage() override;

    static QString toPrettyJson(const QByteArray &raw);
    bool waitForAck(int generation, int timeoutMs);
    bool pushRequestOnce(const QByteArray &payload, int generation);

    CFactoryTestProtocol *m_uart_ch = nullptr;
    CommDebugReceiver *m_receiver = nullptr;
    int m_sendCount = 0;
    int m_ackCount = 0;
    std::mutex m_serialTxMutex;
    std::mutex m_ackWaitMutex;
    std::condition_variable m_ackCv;
    std::atomic<bool> m_inFlight{false};
    std::atomic<int> m_txGeneration{0};
    std::atomic<int> m_ackedGeneration{0};
};

#endif // FACTORYTESTMODULE_COMM_DEBUG_MANAGE_H
