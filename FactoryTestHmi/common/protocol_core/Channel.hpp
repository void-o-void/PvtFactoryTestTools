//
// Created by panshiquan on 2026/3/19.
//

#ifndef MYPROJECT_CCHANNEL_H
#define MYPROJECT_CCHANNEL_H
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <string>
#include <cstring>
#include <utility>
#include <unistd.h>
#include "Utils.hpp"

class Channel {
public:
    Channel(std::string name, const int timeout = 1000) : m_name(std::move(name)), m_timeout(timeout) {}
    Channel(int fd, const int timeout = 1000) : m_name(std::to_string(fd)), m_fd(fd), m_timeout(timeout) {}

    virtual ~Channel() {
        if (m_fd > 0) {
            close(m_fd);
        }
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    bool readnData(uint8_t *data, int len) {
        if (len <= 0)
            return true;
        if (!m_is_run)
            return false;

        int received = 0;
        while (received < len) {
            int ret = readData(data + received, len - received);
            if (ret <= 0) {
                return false;
            }
            received += ret;
        }
        return true;
    }

    virtual int readData(uint8_t *data, int maxlen) {
        return read(m_fd, data, maxlen);
    };

    virtual bool writeData(const uint8_t *data, int len) {
        if (!m_is_run || len <= 0) {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_write_mutex);
        int total = 0;

        while (total < len && m_is_run) {
            int remaining = len - total;
            int length = write(m_fd, data + total, remaining);

            if (length > 0) {
                total += length;
            } else if (length == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } else {
                return false;  // 返回false
            }
        }

        return total == len ? true : false;
    }


    virtual std::string descr() const{
        return m_name;
    }

    virtual bool init() {
        return true;
    }

    virtual bool unInit() {
        return true;
    };

protected:
    int m_fd = -1;
    std::atomic_bool m_is_run{false};

protected:
    std::string m_name;
    std::mutex m_write_mutex;
    int m_timeout = 5;
};

#endif //MYPROJECT_CCHANNEL_H
