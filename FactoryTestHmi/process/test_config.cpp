//
// Created by panshiquan on 2026/5/10.
//
#include "test_config.hpp"
#include <QFile>

// ---------- Config ----------
Config::Config() {
    // 启动时加载默认项目
    QJsonObject cfgObj = jsonFromFile(QString(PROJECT_DIR) + "/config/config.json");
    QString proj = cfgObj["current_project"].toString("P39");
    loadProject(proj);
}

void Config::loadProject(const QString& projectName) {
    QString filePath = QString(PROJECT_DIR) + "/config/" + projectName + ".json";
    QJsonObject obj = jsonFromFile(filePath);
    m_doc = QJsonDocument(obj);
    m_currentProject = projectName;

    // 更新 config.json 中的当前项目记录
    QJsonObject rootCfg;
    rootCfg["current_project"] = projectName;
    jsonToFile(QString(PROJECT_DIR) + "/config/config.json", rootCfg);
}

// ---------- 获取基础信息（直接解析）----------
SFactoryConfig Config::factoryConfig() const {
    SFactoryConfig cfg;
    cfg.fromJson(rootObj()["factory"].toObject());
    return cfg;
}

SStationConfig Config::stationConfig() const {
    SStationConfig cfg;
    cfg.fromJson(rootObj()["station"].toObject());
    return cfg;
}

SSerialConfig Config::connectSerial() const {
    SSerialConfig cfg;
    cfg.fromJson(rootObj()["connect_serial"].toObject());
    return cfg;
}

SSerialConfig Config::debugSerial() const {
    SSerialConfig cfg;
    cfg.fromJson(rootObj()["debug_serial"].toObject());
    return cfg;
}

QMap<QString, SEnvItem> Config::envItems() const {
    QMap<QString, SEnvItem> map;
    QJsonArray arr = rootObj()["env_items"].toArray();
    for (const auto& val : arr) {
        SEnvItem item(val.toObject());
        map.insert(item.descr, item);
    }
    return map;
}

QVector<TestConfigItem> Config::allTestItems() const {
    QVector<TestConfigItem> items;
    QJsonArray arr = rootObj()["test_items"].toArray();
    for (const auto& val : arr) {
        items.append(TestConfigItem(val.toObject()));
    }
    return items;
}

QVector<TestRunItem> Config::enabledTestPlan() const {
    QVector<TestRunItem> plan;
    QJsonArray arr = rootObj()["test_items"].toArray();
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        TestConfigItem cfg(obj);
        if (!cfg.value) continue;

        TestRunItem ti;
        ti.id          = cfg.code;
        ti.name        = cfg.descr;
        ti.testCode    = QString::number(cfg.code);
        ti.status      = "waiting";
        ti.duration    = "0s";
        ti.message     = "等待测试";
        ti.result      = "--";
        ti.timeoutMs   = cfg.timeout * 1000;
        ti.maxRetries  = cfg.retries;
        ti.currentRetry = 0;
        ti.active      = false;
        plan.append(ti);
    }
    return plan;
}

void Config::saveTestItems(const QVector<TestConfigItem>& items) {
    QJsonObject obj = rootObj();
    QJsonArray arr;
    for (const auto& item : items) {
        QJsonObject o;
        o["code"]    = item.code;
        o["sn"]      = item.sn;
        o["value"]   = item.value;
        o["descr"]   = item.descr;
        o["timeout"] = item.timeout;
        o["Retries"] = item.retries;
        arr.append(o);
    }
    obj["test_items"] = arr;
    m_doc.setObject(obj);
    QString filePath = QString(PROJECT_DIR) + "/config/" + m_currentProject + ".json";
    jsonToFile(filePath, obj);
}

QJsonObject Config::jsonFromFile(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

void Config::jsonToFile(const QString& fileName, const QJsonObject& obj) {
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}