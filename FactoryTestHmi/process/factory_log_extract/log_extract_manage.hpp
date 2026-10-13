#ifndef LOG_EXTRACT_MANAGE_HPP
#define LOG_EXTRACT_MANAGE_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <atomic>
#include <mutex>
#include <thread>

#include "common.hpp"
#include "adb_device_query.hpp"

class QProcess;

/**
 * logcfg.json 多平台读取封装。
 * 结构：default_platform + platforms[{id,name,remote_path,systems}]
 */
class LogCfg {
public:
    static bool load(QString *errMsg = nullptr);
    static bool reload(QString *errMsg = nullptr);

    static QString configPath();

    static QStringList systemNames();
    static QStringList platformIds();
    static QStringList platformNames();
    static QString activePlatformId();
    static bool setActivePlatform(const QString &id);

    /** 当前平台下某系统配置的全部型号 */
    static QStringList configuredDevices(const QString &system);
    /** 非 ip:port 的直连型号 */
    static QStringList directDevices(const QString &system);
    /** ip:port 的转发型号 */
    static QStringList forwardDevices(const QString &system);

    static QString adbPath();
    static QString remotePath();
    /** 当前平台读 USB_SWITCH 用的工具名：mt8676=sysenv_test，mt8678=sysenv-test */
    static QString sysenvTool();
    static QString localRoot();
    /** 本地端口 → 远端端口：7665→6665，7666→6666，7667→6667 */
    static int forwardRemotePort(int localPort);

    static bool isIpPortSerial(const QString &serial, QString *host = nullptr, int *port = nullptr);

private:
    LogCfg() = delete;

    struct State {
        QJsonObject root;
        QString path;
        QString activePlatformId;
        bool loaded = false;
    };
    static State &state();

    static QString resolveConfigPath();
    //static QJsonObject defaultConfig();
    static QString normalizeSystemKey(const QString &system);
    static bool loadFromFile(const QString &path, QString *errMsg);
    static QJsonObject activePlatformObject();
    static void ensureActivePlatform();
};

inline LogCfg::State &LogCfg::state()
{
    static State s;
    return s;
}

inline QString LogCfg::resolveConfigPath()
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

inline void LogCfg::ensureActivePlatform()
{
    if (!state().activePlatformId.isEmpty()) {
        for (const QJsonValue &v : state().root.value(QStringLiteral("platforms")).toArray()) {
            if (v.toObject().value(QStringLiteral("id")).toString() == state().activePlatformId)
                return;
        }
    }
    const QString def = state().root.value(QStringLiteral("default_platform")).toString(QStringLiteral("mt8676"));
    const QJsonArray platforms = state().root.value(QStringLiteral("platforms")).toArray();
    for (const QJsonValue &v : platforms) {
        if (v.toObject().value(QStringLiteral("id")).toString() == def) {
            state().activePlatformId = def;
            return;
        }
    }
    if (!platforms.isEmpty())
        state().activePlatformId = platforms.first().toObject().value(QStringLiteral("id")).toString();
    else
        state().activePlatformId = QStringLiteral("mt8676");
}

inline bool LogCfg::loadFromFile(const QString &path, QString *errMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg)
            *errMsg = QStringLiteral("无法读取配置: %1").arg(path);
        //state().root = defaultConfig();
        state().path = path;
        state().activePlatformId.clear();
        state().loaded = true;
        ensureActivePlatform();
        return false;
    }

    QJsonParseError pe{};
    const auto doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errMsg)
            *errMsg = QStringLiteral("配置 JSON 解析失败: %1").arg(pe.errorString());
        //state().root = defaultConfig();
        state().path = path;
        state().activePlatformId.clear();
        state().loaded = true;
        ensureActivePlatform();
        return false;
    }

    state().root = doc.object();
    // 兼容旧版无 platforms 的单层 systems
    if (!state().root.contains(QStringLiteral("platforms"))) {
        QJsonObject legacy = state().root;
        QJsonObject plat{
            {"id", "mt8676"},
            {"name", "MT8676"},
            {"remote_path", legacy.value(QStringLiteral("remote_path")).toString(QStringLiteral("/data/debuglogger/mobilelog"))},
            {"systems", legacy.value(QStringLiteral("systems")).toObject()}
        };
        state().root = QJsonObject{
            {"adb_path", legacy.value(QStringLiteral("adb_path")).toString(QStringLiteral("adb"))},
            {"local_root", legacy.value(QStringLiteral("local_root")).toString()},
            {"default_platform", "mt8676"},
            {"platforms", QJsonArray{plat}}
        };
    }
    state().path = path;
    state().activePlatformId.clear();
    state().loaded = true;
    ensureActivePlatform();
    return true;
}

inline bool LogCfg::load(QString *errMsg)
{
    if (state().loaded)
        return true;
    return reload(errMsg);
}

inline bool LogCfg::reload(QString *errMsg)
{
    const QString path = resolveConfigPath();
    const QString keepId = state().activePlatformId;
    const bool ok = loadFromFile(path, errMsg);
    if (!keepId.isEmpty())
        setActivePlatform(keepId);
    return ok;
}

inline QString LogCfg::configPath()
{
    load();
    return state().path;
}

inline QStringList LogCfg::systemNames()
{
    return {QStringLiteral("YOCTO"), QStringLiteral("TBOX"), QStringLiteral("Android")};
}

inline QString LogCfg::normalizeSystemKey(const QString &system)
{
    for (const QString &name : systemNames()) {
        if (name.compare(system, Qt::CaseInsensitive) == 0)
            return name;
    }
    return system.trimmed();
}

inline QJsonObject LogCfg::activePlatformObject()
{
    load();
    ensureActivePlatform();
    for (const QJsonValue &v : state().root.value(QStringLiteral("platforms")).toArray()) {
        const QJsonObject obj = v.toObject();
        if (obj.value(QStringLiteral("id")).toString() == state().activePlatformId)
            return obj;
    }
    return {};
}

inline QStringList LogCfg::platformIds()
{
    load();
    QStringList ids;
    for (const QJsonValue &v : state().root.value(QStringLiteral("platforms")).toArray()) {
        const QString id = v.toObject().value(QStringLiteral("id")).toString().trimmed();
        if (!id.isEmpty())
            ids << id;
    }
    return ids;
}

inline QStringList LogCfg::platformNames()
{
    load();
    QStringList names;
    for (const QJsonValue &v : state().root.value(QStringLiteral("platforms")).toArray()) {
        const QJsonObject obj = v.toObject();
        const QString name = obj.value(QStringLiteral("name")).toString().trimmed();
        const QString id = obj.value(QStringLiteral("id")).toString().trimmed();
        names << (name.isEmpty() ? id : name);
    }
    return names;
}

inline QString LogCfg::activePlatformId()
{
    load();
    ensureActivePlatform();
    return state().activePlatformId;
}

inline bool LogCfg::setActivePlatform(const QString &id)
{
    load();
    const QString want = id.trimmed();
    if (want.isEmpty())
        return false;
    for (const QJsonValue &v : state().root.value(QStringLiteral("platforms")).toArray()) {
        if (v.toObject().value(QStringLiteral("id")).toString() == want) {
            state().activePlatformId = want;
            return true;
        }
    }
    return false;
}

inline bool LogCfg::isIpPortSerial(const QString &serial, QString *host, int *port)
{
    const int idx = serial.lastIndexOf(':');
    if (idx <= 0)
        return false;
    bool ok = false;
    const int p = serial.mid(idx + 1).toInt(&ok);
    if (!ok || p <= 0)
        return false;
    if (host) *host = serial.left(idx);
    if (port) *port = p;
    return true;
}

inline QStringList LogCfg::configuredDevices(const QString &system)
{
    load();
    const QString key = normalizeSystemKey(system);
    const QJsonArray arr = activePlatformObject().value(QStringLiteral("systems")).toObject().value(key).toArray();
    QStringList list;
    for (const QJsonValue &v : arr) {
        const QString s = v.toString().trimmed();
        if (!s.isEmpty())
            list << s;
    }
    return list;
}

inline QStringList LogCfg::directDevices(const QString &system)
{
    QStringList list;
    for (const QString &s : configuredDevices(system)) {
        if (!isIpPortSerial(s))
            list << s;
    }
    return list;
}

inline QStringList LogCfg::forwardDevices(const QString &system)
{
    QStringList list;
    for (const QString &s : configuredDevices(system)) {
        if (isIpPortSerial(s))
            list << s;
    }
    return list;
}

inline QString LogCfg::adbPath()
{
    load();
    const QString p = state().root.value(QStringLiteral("adb_path")).toString(QStringLiteral("adb")).trimmed();
    return p.isEmpty() ? QStringLiteral("adb") : p;
}

inline QString LogCfg::remotePath()
{
    load();
    const QString fromPlat = activePlatformObject().value(QStringLiteral("remote_path")).toString().trimmed();
    if (!fromPlat.isEmpty())
        return fromPlat;
    const QString p = state().root.value(QStringLiteral("remote_path"))
                          .toString(QStringLiteral("/data/debuglogger/mobilelog")).trimmed();
    return p.isEmpty() ? QStringLiteral("/data/debuglogger/mobilelog") : p;
}

inline QString LogCfg::sysenvTool()
{
    load();
    const QString tool = activePlatformObject().value(QStringLiteral("sysenv_tool")).toString().trimmed();
    return tool.isEmpty() ? QStringLiteral("sysenv_test") : tool;
}

inline QString LogCfg::localRoot()
{
    load();
    const QString configured = state().root.value(QStringLiteral("local_root")).toString().trimmed();
    if (!configured.isEmpty())
        return QDir::cleanPath(configured);
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/FactoryLogs"));
}

inline int LogCfg::forwardRemotePort(int localPort)
{
    if (localPort >= 1000)
        return localPort - 1000;
    return 6666;
}

class LogExtractManage : public QObject {
    Q_OBJECT
    DECLARE_SINGLETON(LogExtractManage)

    Q_PROPERTY(bool extracting READ extracting NOTIFY extractingChanged)
    Q_PROPERTY(QString localRoot READ localRoot NOTIFY localRootChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool yoctoOnline READ yoctoOnline NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool tboxOnline READ tboxOnline NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool androidOnline READ androidOnline NOTIFY connectionStatusChanged)
    Q_PROPERTY(QStringList platformIds READ platformIds NOTIFY platformsChanged)
    Q_PROPERTY(QStringList platformNames READ platformNames NOTIFY platformsChanged)
    Q_PROPERTY(QString activePlatformId READ activePlatformId WRITE setActivePlatform NOTIFY activePlatformChanged)

public:
    Q_INVOKABLE void extractAll();
    /** 单独拉取某一系统；saveDir 为空则用 localRoot/系统名 */
    Q_INVOKABLE void extractSystem(const QString &systemName, const QString &saveDir = QString());
    Q_INVOKABLE void clearLocalLogs();
    /** 弹出目录选择框，返回所选路径；取消返回空串 */
    Q_INVOKABLE QString pickDirectory(const QString &startPath = QString());
    /** 某系统当前保存路径（自定义优先，否则默认 FactoryLogs/系统名） */
    Q_INVOKABLE QString systemSavePath(const QString &systemName) const;
    /** 刷新 YOCTO / TBOX / Android 是否已在 adb devices 中（可拉取） */
    Q_INVOKABLE void refreshConnectionStatus();
    Q_INVOKABLE bool isSystemOnline(const QString &systemName) const;

    QStringList platformIds() const { return LogCfg::platformIds(); }
    QStringList platformNames() const { return LogCfg::platformNames(); }
    QString activePlatformId() const { return LogCfg::activePlatformId(); }
    Q_INVOKABLE void setActivePlatform(const QString &id);

    bool extracting() const { return m_extracting.load(); }
    QString localRoot() const { return m_localRoot; }
    QString statusText() const { return m_statusText; }
    bool yoctoOnline() const { return m_yoctoOnline; }
    bool tboxOnline() const { return m_tboxOnline; }
    bool androidOnline() const { return m_androidOnline; }

signals:
    void logMessage(const QString &msg);
    void extractFinished(bool ok, const QString &summary);
    void noDeviceConnected();
    void extractingChanged();
    void localRootChanged();
    void statusTextChanged();
    void localCleared(bool ok, const QString &msg);
    void connectionStatusChanged();
    void connectionRefreshFinished();
    void platformsChanged();
    void activePlatformChanged();

private:
    LogExtractManage(QObject *parent = nullptr);
    ~LogExtractManage() override;

    void setStatus(const QString &text);
    void emitLog(const QString &msg);
    void requestStop();

    bool loadConfig();
    QString adbBin() const;

    QString runAdb(const QStringList &args, int timeoutMs = 60000, int *exitCode = nullptr);
    bool restartAdbServer();
    bool ensureAdbServerOnce();
    bool ensureDeviceReady(const QString &serial);
    bool disconnectIfOffline(const QString &serial);
    bool connectSerial(const QString &serial);
    bool pullSystemLogs(const QString &systemName, const QString &serial, const QString &saveDir);
    QString uniqueTargetPath(const QString &dir, const QString &baseName) const;
    QString mobilelogBaseName(const QString &systemName) const;
    QString resolveSystemDir(const QString &systemName, const QString &saveDir) const;//设置系统得日志保存到那个文件夹
    bool prepareAdbSession();
    bool isIpPortSerial(const QString &serial, QString *host = nullptr, int *port = nullptr) const;

    /** 读 USB_SWITCH 原始值（如 Y/T/A） */
    QString readUsbSwitchRaw();
    /** USB_SWITCH → YOCTO/TBOX/Android，失败返回空 */
    QString activeSystemFromSwitch(const QString &raw) const;
    /** 按 SWITCH / 失败回退策略，返回该系统待尝试的型号列表（有序） */
    QStringList resolveSerialCandidatesForSystem(const QString &systemName);
    /** 兼容：返回候选列表中的第一个 */
    QString resolveSerialForSystem(const QString &systemName);
    /** 按候选顺序尝试连接，成功返回实际连上的 serial，失败返回空 */
    QString connectSystem(const QString &systemName);
    /** 单型号：已连接 / offline 恢复 / 新连 */
    bool trySerialReady(const QString &serial);
    /** 刷新会话级 SWITCH 缓存并打日志 */
    bool refreshUsbSwitchCache();

    void extractAllWorker();
    void extractSystemWorker(const QString &systemName, const QString &saveDir);
    bool probeSystemOnline(const QString &systemName);
    void refreshConnectionWorker();

    QString m_localRoot;
    QString m_statusText;
    QString m_usbSwitchRaw;
    QString m_usbSwitchSystem;
    bool m_yoctoOnline = false;
    bool m_tboxOnline = false;
    bool m_androidOnline = false;

    std::atomic<bool> m_extracting{false};
    std::atomic<bool> m_abort{false};
    std::atomic<bool> m_refreshing{false};
    std::atomic<bool> m_adbServerReady{false};
    std::mutex m_adbMutex;
    QProcess *m_adbProc = nullptr;
    std::thread m_worker;
    std::thread m_refreshWorker;
};

#endif
