//
// Created by LEGION on 2026/4/23.
// 重构：使用 QSerialPort::readyRead 信号 + 条件变量，
// 解决 Windows 上 waitForReadyRead 不可靠导致接收阻塞的问题。
//

#ifndef FACTORYTESTMODULE_QSERIAL_CHANNEL_H
#define FACTORYTESTMODULE_QSERIAL_CHANNEL_H

#include "Channel.hpp"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include <QDebug>
#include <QMetaObject>
#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>

// ============================================================================
// SerialPortWorker — 在专用 QThread 中运行，负责真实串口 I/O
//   - 利用 Qt 事件循环 + readyRead 信号驱动数据接收
//   - 绕过 Windows 下 waitForReadyRead() 的 Overlapped I/O 缺陷
// ============================================================================
class SerialPortWorker : public QObject {
    Q_OBJECT
public:
    SerialPortWorker(const QString              &portName,
                     QSerialPort::BaudRate       baudRate,
                     std::mutex                 &mtx,
                     std::condition_variable    &cv,
                     std::deque<uint8_t>        &byteQueue,
                     std::atomic_bool           &isRun)
        : QObject(nullptr)
        , m_portName(portName)
        , m_baudRate(baudRate)
        , m_mutex(mtx)
        , m_cv(cv)
        , m_byteQueue(byteQueue)
        , m_isRun(isRun) {}

    ~SerialPortWorker() override {
        closePort();
    }

public slots:
    // 在 QThread 的 start() 信号触发后调用，创建并打开串口
    void openPort() {
        m_serial = new QSerialPort(this);   // parent = this，随线程销毁自动清理
        m_serial->setPortName(m_portName);
        m_serial->setBaudRate(m_baudRate);
        m_serial->setDataBits(QSerialPort::Data8);
        m_serial->setParity(QSerialPort::NoParity);
        m_serial->setStopBits(QSerialPort::OneStop);
        m_serial->setFlowControl(QSerialPort::NoFlowControl);

        if (!m_serial->open(QIODevice::ReadWrite)) {
            qWarning() << "SerialPortWorker: open failed:" << m_serial->errorString();
            m_isRun = false;
            m_cv.notify_all();   // 唤醒可能阻塞在 readData 的线程
            return;
        }

        // ★ 核心：用 readyRead 信号驱动接收，不依赖 waitForReadyRead
        connect(m_serial, &QSerialPort::readyRead,
                this,     &SerialPortWorker::onReadyRead);

        m_isRun = true;
        qDebug() << "SerialPortWorker: port opened" << m_portName << m_baudRate;
    }

    // 来自调用方的异步写入
    void writeData(QByteArray data) {
        if (m_serial && m_serial->isOpen()) {
            std::lock_guard<std::mutex> lock(m_writeMutex);
            m_serial->write(data);
            m_serial->waitForBytesWritten(100);
        }
    }

private slots:
    void onReadyRead() {
        if (!m_serial || !m_serial->isOpen()) return;

        QByteArray data = m_serial->readAll();
        if (data.isEmpty()) return;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (int i = 0; i < data.size(); ++i) {
                m_byteQueue.push_back(static_cast<uint8_t>(data[i]));
            }
        }
        m_cv.notify_one();   // 唤醒 readData 中等待的协议线程
    }

private:
    void closePort() {
        if (m_serial) {
            disconnect(m_serial, &QSerialPort::readyRead,
                       this,     &SerialPortWorker::onReadyRead);
            if (m_serial->isOpen()) {
                m_serial->close();
            }
        }
    }

    QSerialPort                *m_serial = nullptr;
    QString                     m_portName;
    QSerialPort::BaudRate       m_baudRate;
    std::mutex                 &m_mutex;
    std::condition_variable    &m_cv;
    std::deque<uint8_t>        &m_byteQueue;
    std::atomic_bool           &m_isRun;
    std::mutex                  m_writeMutex;
};


// ============================================================================
// SerialChannel — 对外保持 Channel 接口不变
//   内部使用 SerialPortWorker + QThread 驱动 I/O
//   readData() 从字节队列取数据，队列空时阻塞等待（条件变量，无 busy-loop）
// ============================================================================
class SerialChannel : public Channel {
public:
    SerialChannel(const QString &portName, QSerialPort::BaudRate baudRate)
        : Channel(-1, 100)
        , m_portName(portName)
        , m_baudRate(baudRate) {}

    ~SerialChannel() override {
        SerialChannel::unInit();
    }

    bool init() override {
        if (m_is_run)
            return true;

        // 清空上次可能的残留
        {
            std::lock_guard<std::mutex> lock(m_byteMutex);
            m_byteQueue.clear();
        }

        m_thread  = new QThread();
        m_worker  = new SerialPortWorker(m_portName, m_baudRate,
                                         m_byteMutex, m_byteCv,
                                         m_byteQueue, m_is_run);
        m_worker->moveToThread(m_thread);

        QObject::connect(m_thread, &QThread::started,
                         m_worker, &SerialPortWorker::openPort);
        QObject::connect(m_thread, &QThread::finished,
                         m_worker, &QObject::deleteLater);

        m_thread->start();

        // 等待工作线程完成串口打开（最多等 3 秒）
        int waited = 0;
        while (!m_is_run && waited < 3000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            waited += 10;
        }

        return m_is_run;
    }

    bool unInit() override {
        m_is_run = false;                   // 先通知所有等待者
        m_byteCv.notify_all();            // 唤醒阻塞在 readData 的线程

        if (m_thread) {
            m_thread->quit();             // 退出事件循环
            if (!m_thread->wait(2000)) {
                m_thread->terminate();    // 超时强制终止
                m_thread->wait();
            }
            delete m_thread;
            m_thread = nullptr;
            m_worker = nullptr;            // 已 deleteLater 或随线程销毁
        }

        // 再次清空队列
        {
            std::lock_guard<std::mutex> lock(m_byteMutex);
            m_byteQueue.clear();
        }

        return true;
    }

    // ----- 读数据：从线程安全字节队列取 -----
    int readData(uint8_t *data, int maxlen) override {
        if (maxlen <= 0)
            return 0;

        std::unique_lock<std::mutex> lock(m_byteMutex);

        // 阻塞等待，直到队列有数据或通道关闭
        m_byteCv.wait(lock, [this]() {
            return !m_byteQueue.empty() || !m_is_run;
        });

        if (!m_is_run && m_byteQueue.empty())
            return -1;

        // 从队列头部取出最多 maxlen 字节
        size_t n = std::min(static_cast<size_t>(maxlen), m_byteQueue.size());
        for (size_t i = 0; i < n; ++i) {
            data[i] = m_byteQueue.front();
            m_byteQueue.pop_front();
        }
        return static_cast<int>(n);
    }

    // ----- 写数据：异步投递到 I/O 线程执行 -----
    bool writeData(const uint8_t *data, int len) override {
        if (!m_is_run || !m_worker || len <= 0)
            return false;

        // 跨线程安全：通过 QueuedConnection 把写入投递到 I/O 线程
        QByteArray ba(reinterpret_cast<const char*>(data), len);

        // 用 BlockingQueuedConnection 等待写入完成（避免竞态）
        QMetaObject::invokeMethod(m_worker, [this, ba]() {
            m_worker->writeData(ba);
        }, Qt::BlockingQueuedConnection);

        return true;
    }

    std::string descr() const override {
        return "Serial Channel";
    }

private:
    QString                  m_portName;
    QSerialPort::BaudRate    m_baudRate;

    // I/O 线程相关
    QThread                 *m_thread = nullptr;
    SerialPortWorker        *m_worker = nullptr;

    // 线程安全字节队列（SerialPortWorker 写入 → 协议线程读取）
    std::mutex               m_byteMutex;
    std::condition_variable  m_byteCv;
    std::deque<uint8_t>      m_byteQueue;
};

#endif // FACTORYTESTMODULE_QSERIAL_CHANNEL_H
