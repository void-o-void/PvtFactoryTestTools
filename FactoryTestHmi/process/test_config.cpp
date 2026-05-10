//
// Created by panshiquan on 2026/5/10.
//
#include "test_config.hpp"

Config * Config::readConfig(const QString &project_name) {
    Config* config = new Config();  // 或 new Config(parent)
    config->loadProjectData(project_name);
    return config;
}

void Config::loadProjectData(const QString &projectName) {
    QString fileName = PROJECT_DIR + QString("/config/") + projectName + ".json";
    QJsonObject obj = jsonFromFile(fileName);
    fromJson(obj);
    current_project = projectName;
}

void Config::changeProject(const QString &project_name) {
    QString fileName = PROJECT_DIR + QString("/config/"+ project_name + ".json");
    fromJson(jsonFromFile(fileName));
    current_project = project_name;

    QJsonObject rootConfig;
    rootConfig["current_project"] = current_project;
    jsonToFile( QString(QString(PROJECT_DIR) + "/config/config.json"), rootConfig);
}

QJsonObject Config::jsonFromFile(const QString &fileName) {
    QFile file(fileName);
    if (! file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        return QJsonObject();
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

void Config::jsonToFile(const QString &fileName, const QJsonObject &obj) {
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void Config::fromJson(const QJsonObject &obj) {
    project_Name = obj["project_name"].toString();
    m_factory_config.fromJson(obj["factory"].toObject());
    m_station_config.fromJson(obj["station"].toObject());
    m_connect_serial.fromJson(obj["connect_serial"].toObject());
    m_debug_serial.fromJson(obj["debug_serial"].toObject());

    m_test_items.clear();
    QJsonArray test_items_array = obj["test_items"].toArray();
    for (auto it = test_items_array.begin(); it != test_items_array.end(); ++it) {
        QJsonObject obj_tmp = it->toObject();
        m_test_items.insert(obj_tmp["code"].toInt() ,STestItem(obj_tmp));
    }

    m_env_items.clear();
    QJsonArray env_items_array = obj["env_items"].toArray();
    for (auto it = env_items_array.begin(); it != env_items_array.end(); ++it) {
        QJsonObject obj_tmp = it->toObject();
        m_env_items.insert(obj_tmp["descr"].toString(),SEnvItem(obj_tmp));
    }
}
