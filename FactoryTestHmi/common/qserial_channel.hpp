//
// Created by LEGION on 2026/4/23.
//

#ifndef FACTORYTESTMODULE_QSERIAL_CHANNEL_H
#define FACTORYTESTMODULE_QSERIAL_CHANNEL_H

#include "Channel.hpp"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <memory>
#include <QString>

class SerialChannel : public Channel {
public:
    SerialChannel(const QString &portName, QSerialPort::BaudRate baud_rate): Channel(-1, 1000), m_portName(portName), m_baudRate(baud_rate) {
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

        m_is_run = true;
        return true;
    }

    bool unInit() override{
        if (m_serial && m_serial->isOpen()) {
            m_serial->close();
        }
        m_is_run = false;
        return true;
    }

    int readData(uint8_t *data, int maxlen) override{
        if (!m_is_run || !m_serial || !m_serial->isOpen())
            return -1;

        // 阻塞等待数据到达，超时使用基类的 m_timeout（毫秒）
        if (!m_serial->waitForReadyRead(m_timeout)) {
            // 超时或出错，返回 -1 表示读取失败
            return -1;
        }

        // 读取数据（非阻塞，因为 wait 已保证有数据）
        qint64 bytesRead = m_serial->read(reinterpret_cast<char*>(data), maxlen);
        if (bytesRead <= 0) {
            return -1;
        }
        return static_cast<int>(bytesRead);
    }

    bool writeData(const uint8_t *data, int len) override{
        if (!m_is_run || !m_serial || !m_serial->isOpen() || len <= 0)
            return false;

        std::lock_guard<std::mutex> lock(m_write_mutex);  // 复用基类的互斥锁

        qint64 totalWritten = 0;
        while (totalWritten < len && m_is_run) {
            qint64 written = m_serial->write(reinterpret_cast<const char*>(data + totalWritten),
                                             len - totalWritten);
            if (written < 0) {
                return false;
            }
            totalWritten += written;

            // 等待底层发送缓冲区清空（可选，保证数据真正发出）
            if (totalWritten < len) {
                if (!m_serial->waitForBytesWritten(m_timeout)) {
                    return false;
                }
            }
        }
        return totalWritten == len;
    }

    std::string descr() const override;

private:
    std::unique_ptr<QSerialPort> m_serial;
    QString m_portName;
    qint32 m_baudRate = QSerialPort::Baud115200; // 可根据需要扩展设置
    // 其他参数可类似添加
};


#endif //FACTORYTESTMODULE_QSERIAL_CHANNEL_H
