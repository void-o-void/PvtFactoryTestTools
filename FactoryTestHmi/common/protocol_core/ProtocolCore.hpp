//
// Created by void on 2026/3/19.
//

#ifndef CPROTOCOL_H
#define CPROTOCOL_H

#include <mutex>
#include <thread>

#include "Channel.hpp"
#include "BlockingQueu.hpp"
#include "utils.hpp"

template<typename T>
class CProtocol {
public:
    struct SProtocolConfig {
        EByteSort mode;
        uint8_t *head;
        int head_len;

        int fun_len;
        int len_index;
        int len_len;
    };

    struct SDataPacket {
        uint8_t *data;
        int data_len;

        uint8_t *fundata;
        int fun_len;

        uint8_t crc;
    };

    CProtocol(Channel *channel) : m_channel(channel), m_running(false) {
        if (m_channel != nullptr) {
            m_channel->init();
        }
    }

    virtual ~CProtocol() {
        m_running = false;
        if (m_worker.joinable()) {
            m_worker.join();
        }

        if (m_channel != nullptr) {
            m_channel->unInit();
            m_running = false;
            delete m_channel;
        }
    }

    static uint8_t calculateChecksum(const uint8_t *data, int length) {
        uint8_t sum = 0;
        for (int i = 0; i < length; ++i) {
            sum += data[i];
        }
        return sum;
    }

    virtual T pull() {
        return m_queue.pop();
    }

    void push(T msg) {
        int data_len = 0;
        uint8_t *data = encode(msg, &data_len);
        if (data == nullptr) {
            return;
        }

        //std::cout << "send msg: " << CUtils::formatHexToStr(data, data_len) << std::endl;

        m_channel->writeData(data, data_len);
        delete data;
    }

    bool start() {
        m_worker = std::thread([this]() {
            enum ParseState {
                E_HEAD,
                E_FUN,
                E_BODY
            };

            m_cfg = protocolCfg();

            m_running = true;
            ParseState state = E_HEAD;
            SDataPacket pkg;
            while (m_running) {
                switch (state) {
                    case E_HEAD: {
                        pkg = SDataPacket();
                        uint8_t head[m_cfg.head_len];
                        for (int i = 0; i < m_cfg.head_len; i++) {
                            if (m_channel->readData(head + i, 1) != 1 || head[i] != m_cfg.head[i]) {
                                break;
                            }
                            if (i == m_cfg.head_len - 1) {
                                state = E_FUN;
                            }
                        }
                    }
                    break;
                    case E_FUN: {
                        pkg.fundata = new uint8_t[m_cfg.fun_len];
                        if (! m_channel->readnData(pkg.fundata, m_cfg.fun_len)) {
                            state = E_HEAD;
                            break;
                        }

                        pkg.data_len = CUtils::intFromBytes(pkg.fundata + m_cfg.len_index, m_cfg.len_len, m_cfg.mode);
                        state = E_BODY;
                    }
                    break;
                    case E_BODY: {
                        pkg.data = new uint8_t[pkg.data_len];
                        if (! m_channel->readnData(pkg.data, pkg.data_len)) {
                            if (pkg.fundata != nullptr) {
                                delete [] pkg.fundata;
                            }
                            state = E_HEAD;
                            break;
                        }

                        T msg = decode(pkg);
                        m_queue.push(msg);
                        state = E_HEAD;
                    }
                    break;
                }
            }
        });
        return true;
    }

    bool reset() {
        m_running = false;
        if (m_worker.joinable()) {
            m_worker.join();
        }
        return true;
    }

     bool isRunning() const{
        return m_running;
    }
protected:
    virtual T decode(SDataPacket &packet) = 0;
    virtual uint8_t* encode(T &msg, int *len) = 0;
    virtual const SProtocolConfig protocolCfg() = 0;

    SProtocolConfig m_cfg;
private:
    std::thread m_worker;
    Channel *m_channel;
    bool m_running;
    BlockingQueue<T> m_queue;
};


#endif //CPROTOCOL_H
