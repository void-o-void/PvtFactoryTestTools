//
// Created by panshiquan on 2026/4/10.
//

#ifndef SIMPLEPROTOCOL_CUTILS_H
#define SIMPLEPROTOCOL_CUTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

enum EByteSort {
    E_BIG,
    E_LATTLE
};

class CUtils {
public:
    static std::string formatHexToStr(const uint8_t* data, const size_t length, const std::string& separator = " ") {
        std::ostringstream oss;
        for (size_t i = 0; i < length; i++) {
            if (i > 0) {
                oss << separator;
            }
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    static int intFromBytes(const uint8_t* data, size_t byte_count, EByteSort mode) {
        int result = 0;

        if (byte_count > 4) {
            byte_count = 4;
        }

        if (mode == E_BIG) {
            for (size_t i = 0; i < byte_count; i++) {
                result |= (int)data[i] << (8 * (byte_count - 1 - i));
            }
        } else {
            for (size_t i = 0; i < byte_count; i++) {
                result |= (int)data[i] << (8 * i);
            }
        }

        return result;
    }

    static std::vector<std::string> split_lines(const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line)) {
            // getline 默认以 '\n' 为分隔符，同时会自动丢弃末尾的 '\r'（如果存在）
            lines.push_back(line);
        }
        return lines;
    }
};

#endif //SIMPLEPROTOCOL_CUTILS_H
