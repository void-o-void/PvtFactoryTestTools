//
// Created by LEGION on 2026/4/23.
//

#ifndef FACTORYTESTMODULE_QSERIAL_CHANNEL_H
#define FACTORYTESTMODULE_QSERIAL_CHANNEL_H

#include "Channel.hpp"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <memory>
#include <algorithm>
#include <QString>
#include <QDebug>

class SerialChannel : public Channel {
public:
    SerialChannel(const QString &portName, QSerialPort::BaudRate baud_rate): Channel(-1, 100), m_portName(portName), m_baudRate(baud_rate) {
        m_serial = std::make_unique<QSerialPort>();
    }
    ~SerialChannel() override{ SerialChannel::unInit(); }

    // 覆写虚函数
    bool init() override {
        if (m_is_run)
            return true;

        m_serial->setPortName(m_portName);
        m_serial->setBaudRate(m_baudRate);
        m_serial->setDataBits(QSerialPort::Data8);
        m_serial->setParity(QSerialPort::NoParity);
        m_serial->setStopBits(QSerialPort::OneStop);
        m_serial->setFlowControl(QSerialPort::NoFlowControl);

        if (!m_serial->open(QIODevice::ReadWrite)) {
            qDebug() << "SerialChannel: open failed:" << m_serial->errorString();
            return false;
        }

        m_readPos = 0;
        m_readSize = 0;
        m_is_run = true;
        return true;
    }

    bool unInit() override {
        m_is_run = false;  // 先置标志，让阻塞中的 readData 退出
        if (m_serial && m_serial->isOpen()) {
            m_serial->close();
        }
        return true;
    }

    int readData(uint8_t *data, int maxlen) override{
        if (!m_is_run || !m_serial || !m_serial->isOpen())
            return -1;

        // 内部缓冲区有数据，直接从内存取（零系统调用）
        if (m_readPos < m_readSize) {
            size_t available = m_readSize - m_readPos;
            size_t toRead = std::min(static_cast<size_t>(maxlen), available);
            memcpy(data, m_readBuffer + m_readPos, toRead);
            m_readPos += toRead;
            return static_cast<int>(toRead);
        }

        // 缓冲区空了，等待串口数据
        while (m_is_run) {
            if (!m_serial->waitForReadyRead(100)) {
                continue;  // 超时，检查 m_is_run 后继续
            }

            // 直接用大缓冲区 read()，一次读空串口驱动缓冲区所有字节
            // 不依赖 bytesAvailable()，避免 Windows QSerialPort 行为不一致
            qint64 n = m_serial->read(reinterpret_cast<char*>(m_readBuffer), kReadBufSize);
            if (n < 0) {
                qDebug() << "SerialChannel::readData read error:" << m_serial->errorString();
                return -1;
            }
            if (n == 0) {
                // 罕见：waitForReadyRead 返回 true 但没数据，重试
                continue;
            }

            m_readSize = static_cast<size_t>(n);
            m_readPos  = 0;

            size_t toRead = std::min(static_cast<size_t>(maxlen), m_readSize);
            memcpy(data, m_readBuffer, toRead);
            m_readPos = toRead;
            return static_cast<int>(toRead);
        }

        return -1;  // m_is_run == false
    }

    bool writeData(const uint8_t *data, int len) override{
        if (!m_is_run || !m_serial || !m_serial->isOpen() || len <= 0)
            return false;

        std::lock_guard<std::mutex> lock(m_write_mutex);

        qint64 totalWritten = 0;
        while (totalWritten < len && m_is_run) {
            qint64 written = m_serial->write(reinterpret_cast<const char*>(data + totalWritten),
                                             len - totalWritten);
            if (written < 0) {
                return false;
            }
            totalWritten += written;

            if (totalWritten < len) {
                if (!m_serial->waitForBytesWritten(m_timeout)) {
                    return false;
                }
            }
        }
        return totalWritten == len;
    }

    std::string descr() const override {
        return "Serial Channel";
    }

private:
    std::unique_ptr<QSerialPort> m_serial;
    QString m_portName;
    qint32 m_baudRate = QSerialPort::Baud115200;

    // 内部读缓冲区：固定大小数组，一次 read() 读空串口驱动缓冲区
    // 后续逐字节从内存取，零系统调用
    static constexpr size_t kReadBufSize = 8192;
    uint8_t m_readBuffer[kReadBufSize]{};
    size_t m_readPos  = 0;   // 当前读取位置
    size_t m_readSize = 0;   // 缓冲区有效字节数
};


#endif //FACTORYTESTMODULE_QSERIAL_CHANNEL_H
