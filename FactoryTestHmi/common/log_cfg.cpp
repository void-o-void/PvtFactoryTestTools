/*
#include "log_cfg.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

// 已合并至 process/factory_log_extract/log_extract_manage.hpp，本文件整份注释保留。

LogCfg::State &LogCfg::state()
{
    static State s;
    return s;
}

QJsonObject LogCfg::defaultConfig()
{
    return QJsonObject{
        {"adb_path", "adb"},
        {"remote_path", "/data/debuglogger/mobilelog"},
        {"local_root", ""},
        {"forward_remote_port", 6666},
        {"systems", QJsonObject{
            {"YOCTO", QJsonArray{"YOCTO"}},
            {"TBOX", QJsonArray{"127.0.0.1:7667"}},
            {"Android", QJsonArray{"127.0.0.1:7666"}}
        }}
    };
}

QString LogCfg::resolveConfigPath()
{
    const QStringList searchRoots = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };
    for (const QString &root : searchRoots) {
        QDir dir(root);
        for (int depth = 0; depth < 10; ++depth) {
            const QString cfg = dir.filePath(QStringLiteral("config/logcfg.json"));
            if (QFile::exists(dir.filePath(QStringLiteral("CMakeLists.txt")))
                && QFile::exists(cfg)) {
                return QDir::cleanPath(cfg);
            }
            if (!dir.cdUp())
                break;
        }
    }

    const QStringList fallbacks = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/config/logcfg.json"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/logcfg.json")
    };
    for (const QString &p : fallbacks) {
        if (QFile::exists(p))
            return QDir::cleanPath(p);
    }
    return QDir::cleanPath(fallbacks.first());
}

bool LogCfg::loadFromFile(const QString &path, QString *errMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg)
            *errMsg = QStringLiteral("无法读取配置: %1").arg(path);
        state().root = defaultConfig();
        state().path = path;
        state().loaded = true;
        return false;
    }

    QJsonParseError pe{};
    const auto doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errMsg)
            *errMsg = QStringLiteral("配置 JSON 解析失败: %1").arg(pe.errorString());
        state().root = defaultConfig();
        state().path = path;
        state().loaded = true;
        return false;
    }

    state().root = doc.object();
    state().path = path;
    state().loaded = true;
    return true;
}

bool LogCfg::load(QString *errMsg)
{
    if (state().loaded)
        return true;
    return reload(errMsg);
}

bool LogCfg::reload(QString *errMsg)
{
    const QString path = resolveConfigPath();
    return loadFromFile(path, errMsg);
}

QString LogCfg::configPath()
{
    load();
    return state().path;
}

QStringList LogCfg::systemNames()
{
    return {QStringLiteral("YOCTO"), QStringLiteral("TBOX"), QStringLiteral("Android")};
}

QString LogCfg::normalizeSystemKey(const QString &system)
{
    for (const QString &name : systemNames()) {
        if (name.compare(system, Qt::CaseInsensitive) == 0)
            return name;
    }
    return system.trimmed();
}

QStringList LogCfg::getDevices(const QString &system)
{
    load();
    const QString key = normalizeSystemKey(system);
    const QJsonArray arr = state().root.value(QStringLiteral("systems")).toObject().value(key).toArray();
    QStringList list;
    for (const QJsonValue &v : arr) {
        const QString s = v.toString().trimmed();
        if (!s.isEmpty())
            list << s;
    }
    return list;
}

QString LogCfg::adbPath()
{
    load();
    const QString p = state().root.value(QStringLiteral("adb_path")).toString(QStringLiteral("adb")).trimmed();
    return p.isEmpty() ? QStringLiteral("adb") : p;
}

QString LogCfg::remotePath()
{
    load();
    const QString p = state().root.value(QStringLiteral("remote_path"))
                          .toString(QStringLiteral("/data/debuglogger/mobilelog")).trimmed();
    return p.isEmpty() ? QStringLiteral("/data/debuglogger/mobilelog") : p;
}

QString LogCfg::localRoot()
{
    load();
    const QString configured = state().root.value(QStringLiteral("local_root")).toString().trimmed();
    if (!configured.isEmpty())
        return QDir::cleanPath(configured);
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/FactoryLogs"));
}

int LogCfg::forwardRemotePort()
{
    load();
    return state().root.value(QStringLiteral("forward_remote_port")).toInt(6666);
}
*/
