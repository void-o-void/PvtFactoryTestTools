//
// Created by panshiquan on 2026/4/13.
//

#ifndef SIMPLEPROTOCOL_CFACTORYTESTPROTOCOL_H
#define SIMPLEPROTOCOL_CFACTORYTESTPROTOCOL_H

#include "ProtocolCore.hpp"
#include "common.hpp"

#include "json.hpp"

#define HEAD 2           // 头部长度
#define INDEX 1          // 索引长度
#define DATA_LENGTH 2    // 数据长度字段长度
#define TYPE 1           // 类型字段长度
#define CHECKSUM 1       // 校验和长度
#define HEADER1 0xAA     // 帧头字节1
#define HEADER2 0x55     // 帧头字节2
#define MAX_BODY_SIZE 550 // 最大Body content长度
#define MIN_BODY_SIZE 3 // 最小Body content长度
#define MAX_PACKET_SIZE (HEAD + INDEX + DATA_LENGTH + TYPE + MAX_BODY_SIZE + CHECKSUM + 10) //加上冗余10个字节
#define PACKAGE_MIN_LENGTH (HEAD + INDEX + DATA_LENGTH + TYPE + CHECKSUM + MIN_BODY_SIZE) // 最小包长度
#define MAX_BUFFER_SIZE (MAX_PACKET_SIZE * 2) // 缓冲区大小

class CFactoryTestProtocol : public CProtocol<MessageEntity> {
public:
    CFactoryTestProtocol(Channel *channel) : CProtocol(channel) {}

    ~CFactoryTestProtocol() override = default;

    using json = nlohmann::json;

    // ==================== 解析函数 ====================

    // 解析 CommonEntity（从 JSON 字符串）
    static CommonEntity *parse_common_entity(const char *json_str) {
        if (!json_str || *json_str == '\0') {
            return nullptr;
        }

        try {
            json j = json::parse(json_str);
            CommonEntity *common = (CommonEntity *) malloc(sizeof(CommonEntity));
            if (!common) return nullptr;

            // 初始化默认值
            common->action = -1;
            common->state = -1;
            common->msg = nullptr;

            // 解析 action
            if (j.contains("action") && j["action"].is_number()) {
                common->action = j["action"].get<int>();
            }

            // 解析 state
            if (j.contains("state") && j["state"].is_number()) {
                common->state = j["state"].get<int>();
            }

            // 解析 msg
            if (j.contains("msg") && j["msg"].is_string()) {
                std::string msg_str = j["msg"].get<std::string>();
                common->msg = strdup(msg_str.c_str());
            }

            return common;
        } catch (const json::exception &e) {
            printf("JSON parse error in parse_common_entity: %s\n", e.what());
            return nullptr;
        }
    }

    static CodeEntity* parseCodeEntity(std::string buf) {
        if (buf.empty()) return nullptr;

        json data = json::parse(buf);
        CodeEntity* code_entity = new CodeEntity();
        code_entity->code = data["code"].get<int>();
        code_entity->common = new CommonEntity;

        // 1. 取出 para 数组，转换为字符串
        std::vector<uint8_t> para_bytes = data["para"].get<std::vector<uint8_t>>();
        std::string inner_json_str(para_bytes.begin(), para_bytes.end());

        // 2. 二次解析得到真正的参数对象
        json inner = json::parse(inner_json_str);
        code_entity->common->action = inner["action"].get<int>();
        code_entity->common->state  = inner["state"].get<int>();

        if (inner.find("msg") != inner.end()) {
            std::string str = inner["msg"].get<std::string>();
            code_entity->common->msg = strdup(str.c_str());
        }
        return code_entity;
    }

    // ==================== 序列化函数 ====================

    // 序列化 CommonEntity 为 JSON 字符串
    static char *serialize_common_entity(const CommonEntity *common) {
        if (!common) return nullptr;

        try {
            json j;
            j["action"] = common->action;
            j["state"] = common->state;
            if (common->msg) {
                j["msg"] = common->msg;
            } else {
                j["msg"] = nullptr;
            }

            std::string json_str = j.dump(); // 紧凑格式（无格式化）
            return strdup(json_str.c_str());
        } catch (const json::exception &e) {
            printf("JSON serialization error in serialize_common_entity: %s\n", e.what());
            return nullptr;
        }
    }

    // 序列化 CodeEntity 为 JSON 字符串
    static char *serialize_code_entity(const CodeEntity *entity) {
        if (!entity) return nullptr;

        try {
            json j;
            j["code"] = entity->code;

            if (entity->common) {
                // 先序列化 CommonEntity 得到 JSON 字符串
                char *common_json_str = serialize_common_entity(entity->common);
                if (!common_json_str) {
                    // 出错时添加空数组
                    j["para"] = json::array();
                } else {
                    // 将字符串中的每个字符转换为字节值，存入数组
                    size_t len = strlen(common_json_str);
                    json para_array = json::array();
                    for (size_t i = 0; i < len; ++i) {
                        para_array.push_back(static_cast<unsigned char>(common_json_str[i]));
                    }
                    j["para"] = para_array;
                    free(common_json_str);
                }
            } else {
                // 无 common 时添加空数组
                j["para"] = json::array();
            }

            std::string json_str = j.dump();
            return strdup(json_str.c_str());
        } catch (const json::exception &e) {
            printf("JSON serialization error in serialize_code_entity: %s\n", e.what());
            return nullptr;
        }
    }

protected:
    const SProtocolConfig protocolCfg() override{
        static auto* head = new uint8_t[2];
        head[0] = 0xAA;
        head[1] = 0x55;
        static SProtocolConfig cfg{
            E_BIG,
            head,
            2,
            4,
            1,
            2
        };
        return cfg;
    }

    MessageEntity decode(SDataPacket &packet) override {
        MessageEntity msg;
        msg.index = packet.fundata[0];
        msg.type = packet.fundata[3];
        msg.data = new char[packet.data_len-1];
        memcpy(msg.data, packet.data, packet.data_len-1);
        msg.data_len = packet.data_len-1;

        std::string buf("aa 55 " + CUtils::formatHexToStr(packet.fundata, 4) + " ");
        buf += CUtils::formatHexToStr(packet.data, packet.data_len);
        std::cout << m_channel->descr() << " recv: " << buf << std::endl;
        return msg;
    }

    uint8_t* encode(MessageEntity &msg, int *len) override {
        size_t data_len = msg.data_len;

        // 计算总长度
        size_t total_len = HEAD + INDEX + DATA_LENGTH + TYPE + data_len + CHECKSUM;
        int data_length = data_len + TYPE;
        // 分配内存
        uint8_t* encoded = (uint8_t*)malloc(total_len);
        if (!encoded) {
            return NULL;
        }

        // 填充数据
        encoded[0] = HEADER1; // HEAD_DATA[0]
        encoded[1] = HEADER2; // HEAD_DATA[1]
        encoded[2] = msg.index & 0xFF; // index
        encoded[3] = (data_length >> 8) & 0xFF; // dataLength high byte
        encoded[4] = data_length & 0xFF; // dataLength low byte
        encoded[5] = msg.type & 0xFF; // type

        // 复制数据部分
        memcpy(encoded + HEAD + INDEX + DATA_LENGTH + TYPE, msg.data, data_len);

        // 计算校验和 (从索引开始到数据结束)
        uint8_t checksum = 0;
        for (size_t i = HEAD; i < HEAD + INDEX + DATA_LENGTH + TYPE + data_len; i++) {
            checksum ^= encoded[i];
        }

        // 设置校验和
        encoded[HEAD + INDEX + DATA_LENGTH + TYPE + data_len] = checksum;

        *len = total_len;
        return encoded;
    }
};

#endif //SIMPLEPROTOCOL_CFACTORYTESTPROTOCOL_H
