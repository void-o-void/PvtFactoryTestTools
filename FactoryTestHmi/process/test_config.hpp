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

#define PROJECT_DIR  "C:\\Users\\LEGION\\Desktop\\work\\FactoryTestModule"

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

struct STestItem {
    int code;
    int sn;
    bool value;
    QString descr;
    void fromJson(const QJsonObject& obj) {
        code = obj["code"].toInt();
        sn = obj["sn"].toInt();
        value = obj["value"].toBool();
        descr = obj["descr"].toString();
    }
    STestItem(const QJsonObject& obj) {
        fromJson(obj);
    }
};

struct SEnvItem {
    QString descr= "";
    QString type;
    QString value;
    int sn;

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


class Config : public QObject {
    Q_OBJECT

    DECLARE_SINGLETON(Config)

    AUTO_PROPERTY(QString, order_no);
    AUTO_PROPERTY(QString, user_no);
    AUTO_PROPERTY(QString, line_name);
    AUTO_PROPERTY(QString, station_name);
    AUTO_PROPERTY(QString, fixture_no);

public:
    static Config* readConfig(const QString& project_name);

    void loadProjectData(const QString& projectName);

    void changeProject(const QString& project_name);

    static QJsonObject jsonFromFile(const QString& fileName);

    static void jsonToFile(const QString& fileName, const QJsonObject& obj);

    void fromJson(const QJsonObject& obj);

    void save() {

    }

private:
    Config() {
        QJsonObject obj =  jsonFromFile( PROJECT_DIR + QString("/config/config.json"));
        changeProject(obj["current_project"].toString());
    }

    ~Config() {}

public:
    QString project_Name = "";            //项目
    SFactoryConfig m_factory_config{};    //工厂参数
    SStationConfig m_station_config{};    //占位参数
    SSerialConfig m_connect_serial{};     //通讯参数
    SSerialConfig m_debug_serial{};       //调试参数

    QMap<QString, SEnvItem> m_env_items{};         //环境配置
    QMap<int, STestItem> m_test_items{};           //测试项

    QString current_project = "P39";
};

#endif