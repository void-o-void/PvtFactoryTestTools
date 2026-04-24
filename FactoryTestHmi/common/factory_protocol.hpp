//
// Created by panshiquan on 2026/4/13.
//

#ifndef SIMPLEPROTOCOL_CFACTORYTESTPROTOCOL_H
#define SIMPLEPROTOCOL_CFACTORYTESTPROTOCOL_H

#include "ProtocolCore.hpp"
#include "common.hpp"

#include "json.hpp"


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

    // 解析 CodeEntity（从 JSON 字符串）
    static CodeEntity *parse_code_entity(const char *json_str) {
        if (!json_str || *json_str == '\0') {
            printf("Empty JSON string\n");
            return nullptr;
        }

        try {
            json j = json::parse(json_str);
            CodeEntity *entity = (CodeEntity *) malloc(sizeof(CodeEntity));
            if (!entity) return nullptr;

            // 解析 code
            if (!j.contains("code") || !j["code"].is_number()) {
                printf("Missing or invalid 'code' field\n");
                free(entity);
                return nullptr;
            }
            entity->code = j["code"].get<int>();

            // 解析 para
            if (j.contains("para")) {
                std::string para_str = j["para"].dump();
                entity->common = parse_common_entity(para_str.c_str());
            } else {
                printf("Missing 'para' field\n");
                entity->common = nullptr;
            }

            return entity;
        } catch (const json::exception &e) {
            printf("JSON parse error in parse_code_entity: %s\n", e.what());
            return nullptr;
        }
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
        msg.data = new char[packet.data_len + 1];
        memcpy(msg.data, packet.data, packet.data_len);
        msg.data[packet.data_len] = '\0';
        msg.data_len = packet.data_len;
        return msg;
    }

    uint8_t* encode(MessageEntity &msg, int *len) override {
        int total_len = 6 + msg.data_len + 1;
        auto* data = new uint8_t[total_len];

        int index = 0;
        data[index] = 0xAA;
        index += 1;
        data[index] = 0x55;
        index += 1;
        data[index] = msg.index & 0xFF;
        index += 1;
        data[index] = (msg.data_len >> 8) & 0xFF;
        index += 1;
        data[index] = msg.data_len & 0xFF;
        index += 1;
        data[index] = msg.type & 0xFF;
        index += 1;

        memcpy(data + index, msg.data, msg.data_len);
        index += msg.data_len;

        uint8_t checksum = 0;
        for (size_t i = 2; i < 6 + msg.data_len; i++) {
            checksum ^= data[i];
        }
        data[index] = checksum;

        *len = total_len;
        return data;
    }
};

#endif //SIMPLEPROTOCOL_CFACTORYTESTPROTOCOL_H
