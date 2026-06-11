//
// Created by panshiquan on 2026/5/10.
//
#include "test_config.hpp"
#include <QFile>
#include <QDir>
#include <QFileInfo>

// ---------- Config ----------
Config::Config() {
    QJsonObject cfgObj = jsonFromFile(QString(PROJECT_DIR) + "/config/config.json");
    QString proj = cfgObj["current_project"].toString("P39");
    loadProject(proj);
}

// ========== 项目管理 ==========
void Config::loadProject(const QString& projectName) {
    QString filePath = QString(PROJECT_DIR) + "/config/" + projectName + ".json";
    QJsonObject obj = jsonFromFile(filePath);
    m_doc = QJsonDocument(obj);
    m_currentProject = projectName;

    QJsonObject rootCfg;
    rootCfg["current_project"] = projectName;
    jsonToFile(QString(PROJECT_DIR) + "/config/config.json", rootCfg);
    emit projectChanged();
}

QStringList Config::projectList() const {
    QStringList list;
    QDir dir(QString(PROJECT_DIR) + "/config");
    for (const auto& info : dir.entryInfoList({"*.json"}, QDir::Files)) {
        QString name = info.baseName();
        if (name != "config") list.append(name);  // 排除 config.json 自身
    }
    return list;
}

void Config::createProject(const QString& name) {
    QString path = QString(PROJECT_DIR) + "/config/" + name + ".json";
    // 复制当前项目作为模板
    QString src = QString(PROJECT_DIR) + "/config/" + m_currentProject + ".json";
    QFile::copy(src, path);
}

void Config::deleteProject(const QString& name) {
    QString path = QString(PROJECT_DIR) + "/config/" + name + ".json";
    QFile::remove(path);
    // 如果删的是当前项目，切到第一个可用项目
    if (name == m_currentProject) {
        QStringList all = projectList();
        if (!all.isEmpty()) loadProject(all.first());
    }
}

void Config::saveCurrentProject() {
    QString path = QString(PROJECT_DIR) + "/config/" + m_currentProject + ".json";
    jsonToFile(path, rootObj());
}

QVariant Config::readFixedField(const QString& section, const QString& key) const {
    QJsonObject sec = rootObj()[section].toObject();
    return sec[key].toVariant();
}

void Config::updateFixedField(const QString& section, const QString& key, const QVariant& value) {
    QJsonObject obj = rootObj();
    QJsonObject sec = obj[section].toObject();
    if (value.typeId() == QMetaType::Bool)
        sec[key] = value.toBool();
    else if (value.typeId() == QMetaType::Int)
        sec[key] = value.toInt();
    else
        sec[key] = value.toString();
    obj[section] = sec;
    m_doc.setObject(obj);
}

// ========== env_items ==========
QVariantList Config::envItemsForQml() const {
    QVariantList list;
    for (auto& item : envItems()) {
        QVariantMap m;
        m["name"] = item.name; m["descr"] = item.descr;
        m["value"] = item.value; m["type"] = item.type;
        m["sn"] = item.sn;
        list.append(m);
    }
    return list;
}

void Config::addEnvItem(const QVariantMap& item) {
    QJsonObject obj = rootObj();
    QJsonArray arr = obj["env_items"].toArray();
    QJsonObject o;
    o["name"] = item["name"].toString();
    o["descr"] = item["descr"].toString();
    o["value"] = item["type"].toString() == "int" ? QJsonValue(item["value"].toInt()) : QJsonValue(item["value"].toString());
    o["type"] = item["type"].toString();
    o["sn"] = arr.size();
    o["en_enum"] = false;
    arr.append(o);
    obj["env_items"] = arr;
    m_doc.setObject(obj);
}

void Config::updateEnvItem(int index, const QVariantMap& item) {
    QJsonObject obj = rootObj();
    QJsonArray arr = obj["env_items"].toArray();
    if (index < 0 || index >= arr.size()) return;
    QJsonObject o = arr[index].toObject();
    if (item.contains("name"))  o["name"] = item["name"].toString();
    if (item.contains("descr")) o["descr"] = item["descr"].toString();
    if (item.contains("value")) {
        if (o["type"].toString() == "int") o["value"] = item["value"].toInt();
        else o["value"] = item["value"].toString();
    }
    if (item.contains("type"))  o["type"] = item["type"].toString();
    arr[index] = o;
    obj["env_items"] = arr;
    m_doc.setObject(obj);
}

void Config::removeEnvItem(int index) {
    QJsonObject obj = rootObj();
    QJsonArray arr = obj["env_items"].toArray();
    if (index < 0 || index >= arr.size()) return;
    arr.removeAt(index);
    obj["env_items"] = arr;
    m_doc.setObject(obj);
}

// ========== test_items ==========
QVariantList Config::testItemsForQml() const {
    QVariantList list;
    for (auto& item : allTestItems()) {
        QVariantMap m;
        m["code"] = item.code; m["sn"] = item.sn;
        m["value"] = item.value; m["descr"] = item.descr;
        m["timeout"] = item.timeout; m["retries"] = item.retries;
        list.append(m);
    }
    return list;
}

void Config::addTestItem(const QVariantMap& item) {
    QJsonObject obj = rootObj();
    QJsonArray arr = obj["test_items"].toArray();
    QJsonObject o;
    o["code"] = item["code"].toInt();
    o["sn"] = arr.size();
    o["value"] = item["value"].toBool();
    o["descr"] = item["descr"].toString();
    o["timeout"] = item["timeout"].toInt();
    o["retries"] = item["retries"].toInt();
    arr.append(o);
    obj["test_items"] = arr;
    m_doc.setObject(obj);
}

void Config::updateTestItem(int index, const QVariantMap& item) {
    QJsonObject obj = rootObj();
    QJsonArray arr = obj["test_items"].toArray();
    if (index < 0 || index >= arr.size()) return;
    QJsonObject o = arr[index].toObject();
    if (item.contains("code"))    o["code"] = item["code"].toInt();
    if (item.contains("value"))   o["value"] = item["value"].toBool();
    if (item.contains("descr"))   o["descr"] = item["descr"].toString();
    if (item.contains("timeout")) o["timeout"] = item["timeout"].toInt();
    if (item.contains("retries")) o["retries"] = item["retries"].toInt();
    arr[index] = o;
    obj["test_items"] = arr;
    m_doc.setObject(obj);
}

void Config::removeTestItem(int index) {
    QJsonObject obj = rootObj();
    QJsonArray arr = obj["test_items"].toArray();
    if (index < 0 || index >= arr.size()) return;
    arr.removeAt(index);
    obj["test_items"] = arr;
    m_doc.setObject(obj);
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

QList<SEnvItem> Config::envItems() const {
    QList<SEnvItem> list;
    QJsonArray arr = rootObj()["env_items"].toArray();
    for (const auto& val : arr) {
        SEnvItem item(val.toObject());
        list.append(item);
    }
    return list;
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
        ti.code        = cfg.code;
        ti.name        = cfg.descr;
        ti.testCode    = QString::number(cfg.code);
        ti.status      = "idle";
        ti.duration    = "--";
        ti.message     = "未开始";
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