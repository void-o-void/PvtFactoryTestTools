//
// Created by panshiquan on 2026/4/15.
//

#ifndef FACTORY_TEST_CPP_COMM_H
#define FACTORY_TEST_CPP_COMM_H

#define PLATFORM_MT8678_3OS 1
#define PLATFORM_MT8676_3OS 2
#define CURR_PLATFORM PLATFORM_MT8678_3OS

enum TestCode {
    FPGA_DSIO = 1,
    FPGA_DSI1 = 2,
    FPGA_DSI2 = 3,
    FPGA_DP = 4,
    FPGA_EDP = 5,
    FPGA_DISP_PWM2 = 6,
    FPGA_DISP_PWM3 = 7,
    FPGA_PWM0 = 8,
    FPGA_PWM3 = 9,
    FPGA_SGMII_ETH0 = 11,
    FPGA_SGMII_ETH1 = 12,

    FLY_MODE = 254,
    VCN15 = 436,
#if CURR_PLATFORM == PLATFORM_MT8678_3OS
    MT8678_SCP_I2C2 = 400, // 17
    MT8678_SCP_I2C3 = 401, // 18
    MT8678_SCP_I2C4 = 402, // 19
    MT8678_SCP_I2C6 = 403, // 21
    MT8678_SPI_0 = 419,
    MT8678_SPI_1 = 420,
    MT8678_SPI_2 = 421,
    MT8678_SPI_3 = 422,
    MT8678_SPI_4 = 423,
    MT8678_SPI_5 = 424,
    MT8678_SPI_7 = 426,
    MT8678_USB_2 = 	430,
    MT8678_USB_3 = 	431,
    MT8678_PCIE	= 435,

    MT8678_ECID = 436,

#elif CURR_PLATFORM == PLATFORM_MT8676_3OS
    MT8676_I2C_3 = 233,
    MT8676_I2C_5 = 234,
    MT8676_I2C_6 = 235,
    MT8676_I2C_12 = 236,
    MT8676_AUD_I2C = 257,
    MT8676_SPI_0 = 237,
    MT8676_SPI_1 = 238,
    MT8676_SPI_2 = 239,
    MT8676_SPI_3 = 240,
    MT8676_SPI_6 = 241,
    MT8676_SPI_7 = 242,
    MT8676_UART = 249,
    MT8676_PCIE = 256,
#endif
    UNDEFINE
};

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
    char* data;
    uint8_t type;
    short data_len;
} MessageEntity;

#define PWM_DELAY_US 1000000 // 1S超时
#define CMD_DELAY_US 1000000 // 1s超时

#endif //FACTORY_TEST_CPP_COMM_H
