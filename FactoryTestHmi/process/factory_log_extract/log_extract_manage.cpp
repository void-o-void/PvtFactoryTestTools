#include "log_extract_manage.hpp"
#include "adb_device_query.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QMetaObject>
#include <chrono>

LogExtractManage::LogExtractManage(QObject *parent)
    : QObject(parent)
{
    loadConfig();
    setStatus("待机");
    // 程序启动时只执行一次 adb kill-server / start-server
    ensureAdbServerOnce();
    // 窗口关闭 / 退出时先打断 adb，避免 join 卡死
    if (qApp) {
        connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
            requestStop();
        });
    }
}

LogExtractManage::~LogExtractManage()
{
    requestStop();
    if (m_worker.joinable())
        m_worker.join();
    if (m_refreshWorker.joinable())
        m_refreshWorker.join();
}

void LogExtractManage::requestStop()
{
    m_abort.store(true);
    std::lock_guard<std::mutex> lock(m_adbMutex);
    if (m_adbProc && m_adbProc->state() != QProcess::NotRunning)
        m_adbProc->kill();
}

void LogExtractManage::setStatus(const QString &text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void LogExtractManage::emitLog(const QString &msg)
{
    emit logMessage(msg);
}

QString LogExtractManage::adbBin() const
{
    return LogCfg::adbPath();
}

bool LogExtractManage::loadConfig()
{
    QString err;
    const bool ok = LogCfg::reload(&err);
    if (!ok)
        emitLog(QString("[WARN] %1，使用内置默认").arg(err));
    else
        emitLog(QString("[OK] 已加载日志配置: %1").arg(LogCfg::configPath()));

    m_localRoot = LogCfg::localRoot();
    emit localRootChanged();

    for (const QString &sys : LogCfg::systemNames())
        QDir().mkpath(m_localRoot + "/" + sys);
    return true;
}

QString LogExtractManage::runAdb(const QStringList &args, int timeoutMs, int *exitCode)
{
    if (m_abort.load()) {
        if (exitCode) *exitCode = -3;
        return QStringLiteral("[ERR] 已取消");
    }

    QProcess proc;
    {
        std::lock_guard<std::mutex> lock(m_adbMutex);
        m_adbProc = &proc;
    }

    proc.setProgram(adbBin());
    proc.setArguments(args);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();
    if (!proc.waitForStarted(5000)) {
        std::lock_guard<std::mutex> lock(m_adbMutex);
        if (m_adbProc == &proc)
            m_adbProc = nullptr;
        if (exitCode) *exitCode = -1;
        return QString("[ERR] 无法启动 adb，请确认 PATH 或 logcfg.adb_path: %1")
            .arg(proc.errorString());
    }

    // 分片等待，便于退出时 kill 能打断
    constexpr int kSliceMs = 500;
    int waited = 0;
    bool finished = false;
    while (waited < timeoutMs) {
        if (m_abort.load()) {
            proc.kill();
            proc.waitForFinished(3000);
            std::lock_guard<std::mutex> lock(m_adbMutex);
            if (m_adbProc == &proc)
                m_adbProc = nullptr;
            if (exitCode) *exitCode = -3;
            return QStringLiteral("[ERR] 已取消");
        }
        if (proc.waitForFinished(kSliceMs)) {
            finished = true;
            break;
        }
        waited += kSliceMs;
    }

    if (!finished && proc.state() != QProcess::NotRunning) {
        proc.kill();
        proc.waitForFinished(3000);
        std::lock_guard<std::mutex> lock(m_adbMutex);
        if (m_adbProc == &proc)
            m_adbProc = nullptr;
        if (exitCode) *exitCode = -2;
        return QStringLiteral("[ERR] adb 命令超时");
    }

    {
        std::lock_guard<std::mutex> lock(m_adbMutex);
        if (m_adbProc == &proc)
            m_adbProc = nullptr;
    }
    if (exitCode)
        *exitCode = proc.exitCode();
    return AdbDeviceQuery::decodeOutput(proc.readAll());
}

bool LogExtractManage::isIpPortSerial(const QString &serial, QString *host, int *port) const
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

QString LogExtractManage::uniqueTargetPath(const QString &dir, const QString &baseName) const
{
    QDir d(dir);
    QString candidate = d.filePath(baseName);
    if (!QFileInfo::exists(candidate))
        return candidate;

    for (int i = 1; i < 10000; ++i) {
        // 重复名: mobilelog-Y(1) —— ASCII 括号
        const QString name = QString("%1(%2)").arg(baseName).arg(i);
        candidate = d.filePath(name);
        if (!QFileInfo::exists(candidate))
            return candidate;
    }
    return d.filePath(baseName + QDateTime::currentDateTime().toString("_yyyyMMdd_hhmmss"));
}

QString LogExtractManage::mobilelogBaseName(const QString &systemName) const
{
    if (systemName.compare(QStringLiteral("YOCTO"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("mobilelog-Y");
    if (systemName.compare(QStringLiteral("TBOX"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("mobilelog-T");
    if (systemName.compare(QStringLiteral("Android"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("mobilelog-A");
    return QStringLiteral("mobilelog");
}

QString LogExtractManage::resolveSystemDir(const QString &systemName, const QString &saveDir) const
{
    const QString trimmed = saveDir.trimmed();
    if (!trimmed.isEmpty())
        return QDir::cleanPath(trimmed);
    return QDir(m_localRoot).filePath(systemName);
}

QString LogExtractManage::systemSavePath(const QString &systemName) const
{
    return resolveSystemDir(systemName, QString());
}

bool LogExtractManage::isSystemOnline(const QString &systemName) const
{
    if (systemName.compare(QStringLiteral("YOCTO"), Qt::CaseInsensitive) == 0)
        return m_yoctoOnline;
    if (systemName.compare(QStringLiteral("TBOX"), Qt::CaseInsensitive) == 0)
        return m_tboxOnline;
    if (systemName.compare(QStringLiteral("Android"), Qt::CaseInsensitive) == 0)
        return m_androidOnline;
    return false;
}

bool LogExtractManage::probeSystemOnline(const QString &systemName)
{
    const QStringList serials = LogCfg::configuredDevices(systemName);
    if (serials.isEmpty())
        return false;

    // 已连接：刷新时不动
    if (!AdbDeviceQuery().getDevices(systemName).isEmpty())
        return true;

    for (const QString &serial : serials) {
        if (m_abort.load())
            return false;

        const QString status = AdbDeviceQuery().deviceStatus(serial);
        // 该型号已是 device：跳过
        if (status.compare(QStringLiteral("device"), Qt::CaseInsensitive) == 0)
            return true;

        // 仅 offline：disconnect 后再重连
        if (status.compare(QStringLiteral("offline"), Qt::CaseInsensitive) == 0) {
            disconnectIfOffline(serial);
            if (m_abort.load())
                return false;
            if (connectSerial(serial)
                && !AdbDeviceQuery().getDevices(systemName).isEmpty())
                return true;
            continue;
        }

        // 未出现：正常尝试连接（不 disconnect）
        if (connectSerial(serial)
            && !AdbDeviceQuery().getDevices(systemName).isEmpty())
            return true;
    }
    return false;
}

void LogExtractManage::refreshConnectionStatus()
{
    if (m_refreshing.load() || m_extracting.load()) {
        emitLog(QStringLiteral("[WARN] 正在忙，稍后再刷新连接状态"));
        return;
    }
    if (m_refreshWorker.joinable())
        m_refreshWorker.join();

    m_refreshing = true;
    m_abort.store(false);
    setStatus(QStringLiteral("刷新连接中…"));
    emitLog(QStringLiteral("[OK] 开始刷新 YOCTO / TBOX / Android 连接状态"));

    m_refreshWorker = std::thread([this]() { refreshConnectionWorker(); });
}

void LogExtractManage::refreshConnectionWorker()
{
    // 刷新不再 kill-server；已连接系统在 probe 里直接跳过
    const bool y = probeSystemOnline(QStringLiteral("YOCTO"));
    const bool t = probeSystemOnline(QStringLiteral("TBOX"));
    const bool a = probeSystemOnline(QStringLiteral("Android"));

    emitLog(QStringLiteral("[OK] YOCTO: %1").arg(y ? QStringLiteral("已连接(绿)") : QStringLiteral("未连接(红)")));
    emitLog(QStringLiteral("[OK] TBOX: %1").arg(t ? QStringLiteral("已连接(绿)") : QStringLiteral("未连接(红)")));
    emitLog(QStringLiteral("[OK] Android: %1").arg(a ? QStringLiteral("已连接(绿)") : QStringLiteral("未连接(红)")));

    QMetaObject::invokeMethod(this, [this, y, t, a]() {
        m_yoctoOnline = y;
        m_tboxOnline = t;
        m_androidOnline = a;
        m_refreshing = false;
        setStatus(QStringLiteral("连接状态已更新"));
        emit connectionStatusChanged();
        emit connectionRefreshFinished();
    }, Qt::QueuedConnection);
}

QString LogExtractManage::pickDirectory(const QString &startPath)
{
    const QString start = startPath.trimmed().isEmpty() ? m_localRoot : startPath;
    return QFileDialog::getExistingDirectory(
        nullptr,
        QStringLiteral("选择日志保存目录"),
        start,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

bool LogExtractManage::disconnectIfOffline(const QString &serial)
{
    const QString status = AdbDeviceQuery().deviceStatus(serial);
    if (status.compare(QStringLiteral("offline"), Qt::CaseInsensitive) != 0)
        return false;

    emitLog(QStringLiteral("[WARN] 检测到 %1 为 offline，执行 adb disconnect").arg(serial));
    int code = 0;
    const QString out = runAdb({"disconnect", serial}, 15000, &code);
    if (m_abort.load())
        return true;
    emitLog(QStringLiteral("[OK] adb disconnect %1 | %2")
                .arg(serial, out.isEmpty() ? QStringLiteral("done") : out));
    return true;
}

bool LogExtractManage::connectSerial(const QString &serial)
{
    if (m_abort.load())
        return false;

    QString host;
    int port = 0;
    if (isIpPortSerial(serial, &host, &port)) {
        int code = 0;
        // YOCTO 7665→6665 / Android 7666→6666 / TBOX 7667→6667
        const int forwardRemote = LogCfg::forwardRemotePort(port);
        QString out = runAdb({
            "-d", "forward",
            QString("tcp:%1").arg(port),
            QString("tcp:%1").arg(forwardRemote)
        }, 15000, &code);
        if (m_abort.load())
            return false;
        emitLog(QString("[OK] forward tcp:%1 -> tcp:%2 | %3")
                    .arg(port).arg(forwardRemote)
                    .arg(out.isEmpty() ? "done" : out));

        out = runAdb({"connect", serial}, 20000, &code);
        if (m_abort.load())
            return false;
        emitLog(QString("[OK] connect %1 | %2").arg(serial, out));

        return AdbDeviceQuery().listAdbDevices().contains(serial);
    }

    return AdbDeviceQuery().listAdbDevices().contains(serial);
}

bool LogExtractManage::ensureDeviceReady(const QString &serial)
{
    if (m_abort.load())
        return false;

    // 已是 device：拉取时也不折腾
    if (AdbDeviceQuery().deviceStatus(serial)
            .compare(QStringLiteral("device"), Qt::CaseInsensitive) == 0)
        return true;

    // 仅 offline：disconnect 后重连
    if (disconnectIfOffline(serial)) {
        if (m_abort.load())
            return false;
        if (connectSerial(serial))
            return true;
        emitLog(QString("[ERR] offline 恢复后仍无法连接: %1").arg(serial));
        return false;
    }

    // 未出现：正常连接
    if (connectSerial(serial))
        return true;

    emitLog(QString("[ERR] 连接后未在 adb devices 中看到可用设备: %1").arg(serial));
    return false;
}

bool LogExtractManage::pullSystemLogs(const QString &systemName, const QString &serial, const QString &saveDir)
{
    if (m_abort.load())
        return false;
    if (!ensureDeviceReady(serial))
        return false;
    if (m_abort.load())
        return false;

    const QString systemDir = resolveSystemDir(systemName, saveDir);
    QDir().mkpath(systemDir);

    const QString target = uniqueTargetPath(systemDir, mobilelogBaseName(systemName));
    emitLog(QString("[OK] 开始拉取 %1 (%2) -> %3")
                .arg(systemName, serial, target));

    int code = 0;
    const QString out = runAdb({
        "-s", serial,
        "pull",
        LogCfg::remotePath(),
        target
    }, 300000, &code);

    if (m_abort.load() || code == -3) {
        emitLog(QString("[WARN] %1 拉取已取消").arg(systemName));
        QDir(target).removeRecursively();
        QFile::remove(target);
        return false;
    }

    if (code != 0 || out.contains("error:", Qt::CaseInsensitive)
        || out.contains("failed", Qt::CaseInsensitive)
        || out.contains("does not exist", Qt::CaseInsensitive)
        || out.contains("No such file", Qt::CaseInsensitive)) {
        emitLog(QString("[ERR] %1 拉取失败: %2").arg(systemName, out));
        QDir(target).removeRecursively();
        QFile::remove(target);
        return false;
    }

    emitLog(QString("[OK] %1 拉取成功: %2").arg(systemName, out.isEmpty() ? target : out));
    return true;
}

bool LogExtractManage::restartAdbServer()
{
    if (m_abort.load())
        return false;

    emitLog(QStringLiteral("[OK] adb kill-server …"));
    int code = 0;
    QString out = runAdb({"kill-server"}, 15000, &code);
    if (m_abort.load())
        return false;
    emitLog(QStringLiteral("[OK] adb kill-server | %1")
                .arg(out.isEmpty() ? QStringLiteral("done") : out));

    emitLog(QStringLiteral("[OK] adb start-server …"));
    out = runAdb({"start-server"}, 20000, &code);
    if (m_abort.load())
        return false;
    emitLog(QStringLiteral("[OK] adb start-server | %1")
                .arg(out.isEmpty() ? QStringLiteral("done") : out));

    // 给 server 一点启动时间
    for (int i = 0; i < 10 && !m_abort.load(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return !m_abort.load();
}

bool LogExtractManage::ensureAdbServerOnce()
{
    if (m_adbServerReady.load())
        return true;
    if (!restartAdbServer())
        return false;
    m_adbServerReady.store(true);
    return true;
}

bool LogExtractManage::prepareAdbSession()
{
    emitLog(QString("[OK] 本地根目录: %1").arg(m_localRoot));

    // kill-server 仅启动时执行过一次，这里不再重复
    const QStringList devices = AdbDeviceQuery().listAdbDevices();
    if (m_abort.load())
        return false;

    emitLog(QString("[OK] adb devices: %1")
                .arg(devices.isEmpty() ? QStringLiteral("(空)") : devices.join(", ")));

    if (devices.isEmpty()) {
        setStatus("无设备连接");
        emit noDeviceConnected();
        emitLog("[ERR] 无设备连接，请检查 USB/ADB 后重试");
        return false;
    }

    int rootCode = 0;
    const QString rootOut = runAdb({"root"}, 20000, &rootCode);
    if (m_abort.load())
        return false;
    emitLog(QString("[OK] adb root | %1")
                .arg(rootOut.isEmpty() ? QStringLiteral("done") : rootOut));
    return true;
}

void LogExtractManage::extractAll()
{
    if (m_extracting.load()) {
        emitLog("[WARN] 正在提取中，请稍候");
        return;
    }

    if (m_worker.joinable())
        m_worker.join();

    m_abort.store(false);
    m_extracting = true;
    emit extractingChanged();
    setStatus("提取中…");

    m_worker = std::thread([this]() { extractAllWorker(); });
}

void LogExtractManage::extractSystem(const QString &systemName, const QString &saveDir)
{
    if (m_extracting.load()) {
        emitLog("[WARN] 正在提取中，请稍候");
        return;
    }

    if (m_worker.joinable())
        m_worker.join();

    m_abort.store(false);
    m_extracting = true;
    emit extractingChanged();
    setStatus(QString("提取 %1…").arg(systemName));

    const QString sys = systemName;
    const QString dir = saveDir;
    m_worker = std::thread([this, sys, dir]() { extractSystemWorker(sys, dir); });
}

void LogExtractManage::extractAllWorker()
{
    int okCount = 0;
    int failCount = 0;

    emitLog("[OK] ========== 开始全量日志提取 ==========");
    if (!prepareAdbSession()) {
        m_extracting = false;
        emit extractingChanged();
        emit extractFinished(false, m_abort.load() ? QStringLiteral("已取消") : QStringLiteral("无设备连接"));
        return;
    }

    for (const QString &sysName : LogCfg::systemNames()) {
        if (m_abort.load())
            break;
        // 只拉当前已连接的配置型号（每个系统通常只有一个在线）
        const QStringList serials = AdbDeviceQuery().getDevices(sysName);
        if (serials.isEmpty()) {
            emitLog(QString("[WARN] %1 当前无已连接型号，跳过").arg(sysName));
            ++failCount;
            continue;
        }
        setStatus(QString("提取 %1…").arg(sysName));
        for (const QString &serial : serials) {
            if (m_abort.load())
                break;
            if (pullSystemLogs(sysName, serial, QString()))
                ++okCount;
            else
                ++failCount;
        }
    }

    if (m_abort.load()) {
        setStatus("已取消");
        emitLog("[WARN] ========== 提取已取消（程序退出） ==========");
        m_extracting = false;
        emit extractingChanged();
        emit extractFinished(false, "已取消");
        return;
    }

    const bool ok = failCount == 0 && okCount > 0;
    const QString summary = QString("完成: 成功 %1，失败 %2").arg(okCount).arg(failCount);
    setStatus(ok ? "提取完成" : (okCount > 0 ? "部分完成" : "提取失败"));
    emitLog(QString("[OK] ========== %1 ==========").arg(summary));

    m_extracting = false;
    emit extractingChanged();
    emit extractFinished(ok, summary);
}

void LogExtractManage::extractSystemWorker(const QString &systemName, const QString &saveDir)
{
    int okCount = 0;
    int failCount = 0;

    emitLog(QString("[OK] ========== 开始单独提取 %1 ==========").arg(systemName));
    const QString dest = resolveSystemDir(systemName, saveDir);
    emitLog(QString("[OK] 保存目录: %1").arg(dest));

    if (!prepareAdbSession()) {
        m_extracting = false;
        emit extractingChanged();
        emit extractFinished(false, m_abort.load() ? QStringLiteral("已取消") : QStringLiteral("无设备连接"));
        return;
    }

    // 只拉当前已连接的配置型号，不对 json 里全部型号逐个尝试
    const QStringList serials = AdbDeviceQuery().getDevices(systemName);
    if (serials.isEmpty()) {
        emitLog(QString("[WARN] %1 当前无已连接型号，无法拉取").arg(systemName));
        setStatus("无已连接型号");
        m_extracting = false;
        emit extractingChanged();
        emit extractFinished(false, "无已连接型号");
        return;
    }

    emitLog(QString("[OK] %1 将拉取已连接型号: %2").arg(systemName, serials.join(", ")));
    setStatus(QString("提取 %1…").arg(systemName));
    for (const QString &serial : serials) {
        if (m_abort.load())
            break;
        if (pullSystemLogs(systemName, serial, saveDir))
            ++okCount;
        else
            ++failCount;
    }

    if (m_abort.load()) {
        setStatus("已取消");
        emitLog("[WARN] ========== 提取已取消（程序退出） ==========");
        m_extracting = false;
        emit extractingChanged();
        emit extractFinished(false, "已取消");
        return;
    }

    const bool ok = failCount == 0 && okCount > 0;
    const QString summary = QString("%1: 成功 %2，失败 %3").arg(systemName).arg(okCount).arg(failCount);
    setStatus(ok ? "提取完成" : (okCount > 0 ? "部分完成" : "提取失败"));
    emitLog(QString("[OK] ========== %1 ==========").arg(summary));

    m_extracting = false;
    emit extractingChanged();
    emit extractFinished(ok, summary);
}

void LogExtractManage::clearLocalLogs()
{
    if (m_extracting.load()) {
        emitLog("[WARN] 提取中无法清空本地日志");
        emit localCleared(false, "提取中无法清空");
        return;
    }

    m_localRoot = LogCfg::localRoot();
    QDir root(m_localRoot);
    if (!root.exists()) {
        emitLog(QString("[OK] 本地目录不存在，无需清空: %1").arg(m_localRoot));
        emit localCleared(true, "目录不存在");
        setStatus("本地已空");
        return;
    }

    bool allOk = true;
    for (const QString &sys : LogCfg::systemNames()) {
        QDir sysDir(root.filePath(sys));
        if (!sysDir.exists())
            continue;
        const auto entries = sysDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for (const QString &name : entries) {
            const QString path = sysDir.filePath(name);
            QFileInfo fi(path);
            bool removed = fi.isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
            if (!removed) {
                allOk = false;
                emitLog(QString("[ERR] 删除失败: %1").arg(path));
            }
        }
        QDir().mkpath(sysDir.absolutePath());
    }

    if (allOk) {
        emitLog(QString("[OK] 已清空本地日志: %1").arg(m_localRoot));
        setStatus("本地日志已清空");
        emit localCleared(true, "清空完成");
    } else {
        setStatus("清空未完全成功");
        emit localCleared(false, "部分删除失败");
    }
}
