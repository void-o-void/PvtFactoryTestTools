//
// Created by panshiquan on 2026/4/15.
//

#ifndef FACTORY_TEST_CPP_COMM_H
#define FACTORY_TEST_CPP_COMM_H

#include <vector>
#include <cstdint>

#define PLATFORM_MT8678_3OS 1
#define PLATFORM_MT8676_3OS 2
#define CURR_PLATFORM PLATFORM_MT8678_3OS

typedef enum {
    TEST_STATUS_UNSTART = 0x01,
    TEST_STATUS_START = 0x02,
    TEST_STATUS_END = 0x03
} TestStatus;

typedef enum {
    TEST_ACTION_SUCCESS = 0x01,
    TEST_ACTION_FAIL = 0x02
} TestResult;

typedef enum {
    TEST_LOCAL = 0x01,
    TEST_FPGA = 0x02
} TestType;

// CommonEntity结构体
typedef struct {
    int action;     // 测试动作 (0:停止测试, 1:开始测试)
    int state;      // 测试状态 (0:FAIL, 1:PASS, 2:未测试, 3:未完成测试中)
    char* msg;      // 消息内容 (UTF-8字符串)
} CommonEntity;

// CodeEntity结构体
typedef struct {
    int code;       // 测试项代码
    CommonEntity* common; // 嵌套的CommonEntity
} CodeEntity;

typedef struct {
    uint8_t index;
    uint8_t type;
    short data_len;
    std::vector<uint8_t> data;   // RAII 自动管理内存
} MessageEntity;

#define DECLARE_SINGLETON(ClassName) \
    public: \
    static ClassName* instance(){ \
        static ClassName instance; \
        return &instance; \
    }; \
    private: \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;


#endif //FACTORY_TEST_CPP_COMM_H
