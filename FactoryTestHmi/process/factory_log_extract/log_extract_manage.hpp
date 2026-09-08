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
 * logcfg.json 读取封装，供日志提取 / 后续 adb 相关功能复用。
 * 路径：优先项目根 config/logcfg.json，备用 exe 旁拷贝。
 * （原 common/log_cfg.hpp + log_cfg.cpp 已合并至此）
 */
class LogCfg {
public:
    /** 加载配置（已加载则直接返回 true） */
    static bool load(QString *errMsg = nullptr);
    /** 强制重新从磁盘读取 */
    static bool reload(QString *errMsg = nullptr);

    static QString configPath();

    /** 系统列表，默认 YOCTO / TBOX / Android */
    static QStringList systemNames();

    /** 取某系统在 logcfg 中配置的全部型号（不含在线过滤） */
    static QStringList configuredDevices(const QString &system);

    static QString adbPath();
    static QString remotePath();
    /** 空配置时返回 exe 同级 FactoryLogs */
    static QString localRoot();
    /** 本地端口 → 远端端口：7665→6665，7666→6666，7667→6667 */
    static int forwardRemotePort(int localPort);

private:
    LogCfg() = delete;

    struct State {
        QJsonObject root;
        QString path;
        bool loaded = false;
    };
    static State &state();

    static QString resolveConfigPath();
    static QJsonObject defaultConfig();
    static QString normalizeSystemKey(const QString &system);
    static bool loadFromFile(const QString &path, QString *errMsg);
};

inline LogCfg::State &LogCfg::state()
{
    static State s;
    return s;
}

inline QJsonObject LogCfg::defaultConfig()
{
    return QJsonObject{
        {"adb_path", "adb"},
        {"remote_path", "/data/debuglogger/mobilelog"},
        {"local_root", ""},
        {"systems", QJsonObject{
            {"YOCTO", QJsonArray{"YOCTO", "127.0.0.1:7665"}},
            {"TBOX", QJsonArray{"127.0.0.1:7667"}},
            {"Android", QJsonArray{"127.0.0.1:7666", "0123456789ABCDEF"}}
        }}
    };
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

inline bool LogCfg::loadFromFile(const QString &path, QString *errMsg)
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

inline bool LogCfg::load(QString *errMsg)
{
    if (state().loaded)
        return true;
    return reload(errMsg);
}

inline bool LogCfg::reload(QString *errMsg)
{
    const QString path = resolveConfigPath();
    return loadFromFile(path, errMsg);
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

inline QStringList LogCfg::configuredDevices(const QString &system)
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

inline QString LogCfg::adbPath()
{
    load();
    const QString p = state().root.value(QStringLiteral("adb_path")).toString(QStringLiteral("adb")).trimmed();
    return p.isEmpty() ? QStringLiteral("adb") : p;
}

inline QString LogCfg::remotePath()
{
    load();
    const QString p = state().root.value(QStringLiteral("remote_path"))
                          .toString(QStringLiteral("/data/debuglogger/mobilelog")).trimmed();
    return p.isEmpty() ? QStringLiteral("/data/debuglogger/mobilelog") : p;
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
    // adb -d forward tcp:7665 tcp:6665 等
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

private:
    LogExtractManage(QObject *parent = nullptr);
    ~LogExtractManage() override;

    void setStatus(const QString &text);
    void emitLog(const QString &msg);
    /** 退出保护：打断正在进行的 adb，尽快结束工作线程 */
    void requestStop();

    bool loadConfig();
    QString adbBin() const;

    QString runAdb(const QStringList &args, int timeoutMs = 60000, int *exitCode = nullptr);
    /** 连接前统一执行一次 adb kill-server / start-server */
    bool restartAdbServer();
    /** 进程生命周期内只执行一次 kill-server/start-server */
    bool ensureAdbServerOnce();
    bool ensureDeviceReady(const QString &serial);
    /** 若型号为 offline：adb disconnect 后再按需重连；返回是否已处理离线 */
    bool disconnectIfOffline(const QString &serial);
    /** IP 型号：forward + connect；USB 型号：仅检查在线 */
    bool connectSerial(const QString &serial);
    bool pullSystemLogs(const QString &systemName, const QString &serial, const QString &saveDir);
    QString uniqueTargetPath(const QString &dir, const QString &baseName) const;
    QString mobilelogBaseName(const QString &systemName) const;
    QString resolveSystemDir(const QString &systemName, const QString &saveDir) const;
    bool prepareAdbSession();
    bool isIpPortSerial(const QString &serial, QString *host = nullptr, int *port = nullptr) const;
    void extractAllWorker();
    void extractSystemWorker(const QString &systemName, const QString &saveDir);
    bool probeSystemOnline(const QString &systemName);
    void refreshConnectionWorker();

    QString m_localRoot;
    QString m_statusText;
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
