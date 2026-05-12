//
// Created by panshiquan on 2026/4/17.
//
#ifndef FACTORYTESTMODULE_TEST_CONFIG_H
#define FACTORYTESTMODULE_TEST_CONFIG_H
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QMetaType>
#include <QMap>
#include "common.hpp"
#include "uicommon.hpp"
#include "test_rt_model.hpp"

#define PROJECT_DIR  "C:\\Users\\panshiquan\\Desktop\\work\\PvtFactoryTestTools\\FactoryTestHmi"

struct SSerialConfig {
    QString com = "COM1";                 //COM口
    int baud_rate = 115200;               //波特率
    uint8_t stop_bits = 1;                //停止位
    uint8_t parity_bits = 1;              //校验位
    void fromJson(const QJsonObject& obj) {
        com = obj["com"].toString();
        baud_rate = obj["baud_rate"].toInt();
    }
};

struct SFactoryConfig {
    QString factory_name = "PVT1";      //工厂名称
    QString mes_ip = "192.168.1.100";      //mes IP
    int   mes_port = 8080;              //mes PORT
    bool mes_enable = false;        //mes 上传
    void fromJson(const QJsonObject& obj) {
        factory_name = obj["factory_name"].toString();
        mes_ip = obj["mes_ip"].toString();
        mes_port = obj["mes_port"].toInt();
        mes_enable = obj["mes_enable"].toBool();
    }
};

struct SStationConfig {
    QString user_no= "Q2503231";            //员工序号
    QString order_no= "20001520";           //工单号
    QString line_name= "FT-0718";           //线别
    QString station_name= "SMT-FT";         //工序名称
    QString fixture_no = "51516";       //夹具编号
    void fromJson(const QJsonObject& obj) {
        user_no = obj["user_no"].toString();
        order_no = obj["order_no"].toString();
        line_name = obj["line_name"].toString();
        station_name = obj["station_name"].toString();
        fixture_no = obj["fixture_no"].toString();
    }
};

struct SEnvItem {
    QString descr= "";
    QString type;
    QString value;
    int sn{};

    bool is_enum = false;
    int index = 0;
    QStringList enum_list;
    SEnvItem(const QJsonObject& obj) { fromJson(obj); }

    void fromJson(const QJsonObject& obj) {
        descr = obj["descr"].toString();
        type = obj["type"].toString();
        sn = obj["sn"].toInt();

        is_enum = obj["en_enum"].toBool();
        if (is_enum) {
            index = obj["index"].toInt();
            QJsonArray enum_array = obj["enum_list"].toArray();
            for (auto it = enum_array.begin(); it != enum_array.end(); ++it) {
                enum_list.append(it->toString());
            }
            value = enum_list.at(index);
        } else {
            if (type == "string") {
                value = obj["value"].toString();
            } else if (type == "int") {
                value = QString::number(obj["value"].toInt());
            }
        }
    }
};

struct TestConfigItem {
    int code{};
    int sn{};
    bool value{};
    QString descr;
    int timeout = 10;   // 秒
    int retries = 0;
    void fromJson(const QJsonObject& obj) {
        this->code = obj["code"].toInt();
        this->sn = obj["sn"].toInt();
        this->value = obj["value"].toBool();
        this->descr = obj["descr"].toString();
        this->timeout = obj["timeout"].toInt();
        this->retries = obj["retries"].toInt();
    }
    TestConfigItem(const QJsonObject& obj){fromJson(obj);};
};


class Config : public QObject {
    Q_OBJECT
    DECLARE_SINGLETON(Config)

public:
    // 切换项目（加载新 JSON）
    void loadProject(const QString& projectName);
    [[nodiscard]] QString currentProject() const { return m_currentProject; }

    // ========== 按需获取方法（不缓存）==========
    // 基础信息（一次性获取多个，可返回结构体，但不存成员）
    [[nodiscard]] SFactoryConfig factoryConfig() const;
    [[nodiscard]] SStationConfig stationConfig() const;
    [[nodiscard]] SSerialConfig connectSerial() const;
    [[nodiscard]] SSerialConfig debugSerial() const;

    // 环境变量
    [[nodiscard]] QMap<QString, SEnvItem> envItems() const;

    // 全量测试项（配置界面用）
    [[nodiscard]] QVector<TestConfigItem> allTestItems() const;

    // 启用的测试项 → 转为测试流程结构体
    [[nodiscard]] QVector<TestRunItem> enabledTestPlan() const;

    // 保存修改后的测试项全量配置到文件（配置界面调用）
    void saveTestItems(const QVector<TestConfigItem>& items);

    // 通用 JSON 文件读写
    static QJsonObject jsonFromFile(const QString& fileName);
    static void jsonToFile(const QString& fileName, const QJsonObject& obj);

private:
    Config();
    ~Config() override = default;
    [[nodiscard]] QJsonObject rootObj() const { return m_doc.object(); }

    QString m_currentProject;
    QJsonDocument m_doc;   // 唯一的数据源
};

#endif