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
    emit platformsChanged();
    emit activePlatformChanged();

    for (const QString &sys : LogCfg::systemNames())
        QDir().mkpath(m_localRoot + "/" + sys);
    return true;
}

void LogExtractManage::setActivePlatform(const QString &id)
{
    if (!LogCfg::setActivePlatform(id)) {
        emitLog(QStringLiteral("[WARN] 未知平台 id: %1").arg(id));
        return;
    }
    m_usbSwitchRaw.clear();
    m_usbSwitchSystem.clear();
    m_localRoot = LogCfg::localRoot();
    for (const QString &sys : LogCfg::systemNames())
        QDir().mkpath(m_localRoot + "/" + sys);
    emit localRootChanged();
    emit activePlatformChanged();
    emitLog(QStringLiteral("[OK] 已切换平台: %1").arg(LogCfg::activePlatformId()));
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
    return LogCfg::isIpPortSerial(serial, host, port);
}

QString LogExtractManage::readUsbSwitchRaw()
{
    int code = 0;
    const QString tool = LogCfg::sysenvTool();
    const QString out = runAdb({
        "-d", "shell", tool, "read", "USB_SWITCH"
    }, 15000, &code);
    if (m_abort.load())
        return {};

    emitLog(QStringLiteral("[OK] adb -d shell %1 read USB_SWITCH").arg(tool));

    // 输出可能多行，取最后一个非空 token
    QString raw = out.trimmed();
    const QStringList lines = raw.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    if (!lines.isEmpty())
        raw = lines.last().trimmed();
    // 有的实现输出 "USB_SWITCH=T" 或仅 "T"
    const int eq = raw.lastIndexOf('=');
    if (eq >= 0)
        raw = raw.mid(eq + 1).trimmed();
    if (!raw.isEmpty())
        raw = raw.left(1);
    return raw.toUpper();
}

QString LogExtractManage::activeSystemFromSwitch(const QString &raw) const
{
    if (raw.compare(QStringLiteral("Y"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("YOCTO");
    if (raw.compare(QStringLiteral("T"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("TBOX");
    if (raw.compare(QStringLiteral("A"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Android");
    return {};
}

bool LogExtractManage::refreshUsbSwitchCache()
{
    m_usbSwitchRaw = readUsbSwitchRaw();
    m_usbSwitchSystem = activeSystemFromSwitch(m_usbSwitchRaw);
    if (m_usbSwitchSystem.isEmpty()) {
        emitLog(QStringLiteral("[WARN] USB_SWITCH 读取失败或未知值: \"%1\"，连接将优先尝试转发型号")
                    .arg(m_usbSwitchRaw.isEmpty() ? QStringLiteral("(空)") : m_usbSwitchRaw));
        return false;
    }
    emitLog(QStringLiteral("[OK] USB_SWITCH=%1 → 当前默认系统: %2")
                .arg(m_usbSwitchRaw, m_usbSwitchSystem));
    return true;
}

QStringList LogExtractManage::resolveSerialCandidatesForSystem(const QString &systemName)
{
    const QString key = systemName.trimmed();
    const QStringList directs = LogCfg::directDevices(key);
    const QStringList forwards = LogCfg::forwardDevices(key);

    QStringList out;
    auto appendUnique = [&out](const QStringList &list) {
        for (const QString &s : list) {
            if (!s.isEmpty() && !out.contains(s))
                out << s;
        }
    };

    // USB_SWITCH 读失败：先直连，再转发
    if (m_usbSwitchSystem.isEmpty()) {
        emitLog(QStringLiteral("[WARN] %1：USB_SWITCH 未知，候选顺序=直连 → 转发")
                    .arg(key));
        appendUnique(directs);
        appendUnique(forwards);
        return out;
    }

    // 与 SWITCH 一致：先直连，再转发兜底
    if (key.compare(m_usbSwitchSystem, Qt::CaseInsensitive) == 0) {
        emitLog(QStringLiteral("[OK] %1 与 USB_SWITCH 一致，候选顺序=直连 → 转发")
                    .arg(key));
        appendUnique(directs);
        appendUnique(forwards);
        return out;
    }

    // 非当前 SWITCH 系统：先转发，再直连兜底
    emitLog(QStringLiteral("[OK] %1 非当前 SWITCH 系统，候选顺序=转发 → 直连")
                .arg(key));
    appendUnique(forwards);
    appendUnique(directs);
    return out;
}

QString LogExtractManage::resolveSerialForSystem(const QString &systemName)
{
    const QStringList candidates = resolveSerialCandidatesForSystem(systemName);
    return candidates.isEmpty() ? QString() : candidates.first();
}

bool LogExtractManage::trySerialReady(const QString &serial)
{
    if (serial.isEmpty() || m_abort.load())
        return false;

    // 已连接：不动
    if (AdbDeviceQuery().deviceStatus(serial)
            .compare(QStringLiteral("device"), Qt::CaseInsensitive) == 0)
        return true;

    const QString status = AdbDeviceQuery().deviceStatus(serial);
    if (status.compare(QStringLiteral("offline"), Qt::CaseInsensitive) == 0) {
        disconnectIfOffline(serial);
        if (m_abort.load())
            return false;
        return connectSerial(serial);
    }

    // 未出现：尝试连接（直连名依赖已在 devices 中；转发会 forward+connect）
    return connectSerial(serial);
}

QString LogExtractManage::connectSystem(const QString &systemName)
{
    const QStringList candidates = resolveSerialCandidatesForSystem(systemName);//adb连接系统方式  根据能否识别到当前系统来的
    if (candidates.isEmpty()) {
        emitLog(QStringLiteral("[WARN] %1 未配置可用型号").arg(systemName));
        return {};
    }

    emitLog(QStringLiteral("[OK] %1 连接候选: %2")
                .arg(systemName, candidates.join(QStringLiteral(", "))));

    for (int i = 0; i < candidates.size(); ++i) {
        if (m_abort.load())
            return {};

        const QString &serial = candidates.at(i);
        const bool isForward = LogCfg::isIpPortSerial(serial);
        emitLog(QStringLiteral("[OK] %1 尝试%2 (%3/%4): %5")
                    .arg(systemName,
                         isForward ? QStringLiteral("转发") : QStringLiteral("直连"),
                         QString::number(i + 1),
                         QString::number(candidates.size()),
                         serial));

        if (trySerialReady(serial)) {//devices状态不动  不在线时执行断开再重新连接
            emitLog(QStringLiteral("[OK] %1 已连接: %2").arg(systemName, serial));
            return serial;
        }

        emitLog(QStringLiteral("[WARN] %1 未能通过 %2 连接")
                    .arg(systemName, serial));
    }

    emitLog(QStringLiteral("[ERR] %1 直连与转发均未连接成功").arg(systemName));
    return {};
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
    return !connectSystem(systemName).isEmpty();
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
    emitLog(QStringLiteral("[OK] 开始刷新 YOCTO / TBOX / Android 连接状态（平台 %1）")
                .arg(LogCfg::activePlatformId()));

    m_refreshWorker = std::thread([this]() { refreshConnectionWorker(); });
}

void LogExtractManage::refreshConnectionWorker()
{
    refreshUsbSwitchCache();

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
    emitLog(QStringLiteral("[OK] 当前平台: %1").arg(LogCfg::activePlatformId()));

    // kill-server 仅启动时执行过一次，这里不再重复
    const QStringList devices = AdbDeviceQuery().listAdbDevices();
    if (m_abort.load())
        return false;

    emitLog(QString("[OK] adb devices: %1")
                .arg(devices.isEmpty() ? QStringLiteral("(空)") : devices.join(", ")));

    if (devices.isEmpty()) {//确认有没有设备连接
        setStatus("无设备连接");
        emit noDeviceConnected();
        emitLog("[ERR] 无设备连接，请检查 USB/ADB 后重试");
        return false;
    }

    refreshUsbSwitchCache();
    if (m_abort.load())
        return false;

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
    if (m_extracting.load()) {//用原子读的方式从内存里拿最新值  保证数据可靠
        emitLog("[WARN] 正在提取中，请稍候");
        return;
    }

    if (m_worker.joinable())//确认线程是否还活着
        m_worker.join();//活着就阻塞等他跑完

    m_abort.store(false);//清掉取消标志位 这次提取从头开始
    m_extracting = true;//将标志位改成提取中
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

        const QString serial = connectSystem(sysName);
        if (serial.isEmpty()) {
            emitLog(QString("[WARN] %1 无法连接，跳过").arg(sysName));
            ++failCount;
            continue;
        }

        setStatus(QString("提取 %1…").arg(sysName));
        if (pullSystemLogs(sysName, serial, QString()))
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
    const QString dest = resolveSystemDir(systemName, saveDir);//设置要保存日志得路径
    emitLog(QString("[OK] 保存目录: %1").arg(dest));

    if (!prepareAdbSession()) {//判断是否有设备连接  没有的话直接进入判断执行拉取取消
        m_extracting = false;
        emit extractingChanged();
        emit extractFinished(false, m_abort.load() ? QStringLiteral("已取消") : QStringLiteral("无设备连接"));
        return;
    }

    const QString serial = connectSystem(systemName);
    if (serial.isEmpty()) {
        emitLog(QString("[WARN] %1 无法连接，无法拉取").arg(systemName));
        setStatus("连接失败");
        m_extracting = false;
        emit extractingChanged();
        emit extractFinished(false, "连接失败");
        return;
    }

    emitLog(QString("[OK] %1 将使用型号: %2").arg(systemName, serial));
    setStatus(QString("提取 %1…").arg(systemName));
    if (pullSystemLogs(systemName, serial, saveDir))
        ++okCount;
    else
        ++failCount;

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
