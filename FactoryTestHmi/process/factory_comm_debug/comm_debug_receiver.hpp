#ifndef FACTORYTESTMODULE_COMM_DEBUG_RECEIVER_H
#define FACTORYTESTMODULE_COMM_DEBUG_RECEIVER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <atomic>
#include <mutex>
#include <thread>

#include "factory_protocol.hpp"

/**
 * 通讯调试「服务器」侧（独立文件）：
 * - 启动后仅从物理串口 RX（协议 decode → pull）收请求
 * - 解析 JSON，整理 ACK 后再经 TX push 回传
 * - 不使用软件队列环回、不直调客户端业务
 */
class CommDebugReceiver : public QObject {
    Q_OBJECT
public:
    explicit CommDebugReceiver(QObject *parent = nullptr);
    ~CommDebugReceiver() override;

    void setChannel(CFactoryTestProtocol *ch);
    void setTxMutex(std::mutex *txMutex);
    /** 客户端新一轮发送：按 generation 去重，避免固定 request_id 误杀合法重发 */
    void prepareForNewRequest(int generation);
    bool start();
    void stop();
    bool isRunning() const { return m_running.load(); }

    void processClientRequest(const QByteArray &raw, bool replyOnSerial);

signals:
    void statusMessage(const QString &msg);
    void jsonReceived(const QString &prettyJson);
    /** generation: 对应本次客户端发送轮次 */
    void ackReplied(int generation);

private:
    void workerLoop();
    static QString toPrettyJson(const QByteArray &raw);
    static QByteArray buildAckBytes(const QByteArray &requestRaw, QString *err);

    CFactoryTestProtocol *m_uart_ch = nullptr;
    std::mutex *m_txMutex = nullptr;
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
    std::atomic<int> m_currentGeneration{0};
    std::atomic<int> m_ackedGeneration{-1};
};

#endif // FACTORYTESTMODULE_COMM_DEBUG_RECEIVER_H
