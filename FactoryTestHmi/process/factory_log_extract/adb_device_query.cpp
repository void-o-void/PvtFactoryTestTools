#include "adb_device_query.hpp"

#include "log_extract_manage.hpp"

#include <QProcess>
#include <QRegularExpression>
#include <QStringConverter>

QString AdbDeviceQuery::decodeOutput(const QByteArray &raw)
{
    if (raw.isEmpty())
        return {};

    // 1) 合法 UTF-8 优先（部分新版 adb / 英文环境）
    {
        QStringDecoder utf8(QStringConverter::Utf8);
        const QString s = utf8(raw);
        if (!utf8.hasError())
            return s.trimmed();
    }

#ifdef Q_OS_WIN
    // 2) 中文 Windows 下 adb 错误信息多为 GBK
    {
        QStringDecoder gbk("GBK");
        if (gbk.isValid()) {
            const QString s = gbk(raw);
            if (!gbk.hasError())
                return s.trimmed();
            // 即使有少量解码问题，GBK 通常仍比乱码可读
            if (!s.isEmpty())
                return s.trimmed();
        }
    }
#endif

    // 3) 系统本地编码兜底
    return QString::fromLocal8Bit(raw).trimmed();
}

QString AdbDeviceQuery::runAdb(const QStringList &args, int timeoutMs) const
{
    QProcess proc;
    proc.setProgram(LogCfg::adbPath());
    proc.setArguments(args);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();
    if (!proc.waitForStarted(5000))
        return QString();
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(3000);
        return QString();
    }
    return decodeOutput(proc.readAll());
}

QStringList AdbDeviceQuery::listAdbDevices() const
{
    const QString out = runAdb({"devices"}, 10000);
    QStringList serials;
    const QStringList lines = out.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("List of devices")))
            continue;
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() >= 2 && parts[1] == QStringLiteral("device"))
            serials << parts[0];
    }
    return serials;
}

QString AdbDeviceQuery::deviceStatus(const QString &serial) const
{
    if (serial.trimmed().isEmpty())
        return {};
    const QString out = runAdb({"devices"}, 10000);
    const QStringList lines = out.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("List of devices")))
            continue;
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() >= 2 && parts[0] == serial)
            return parts[1];
    }
    return {};
}

QStringList AdbDeviceQuery::getDevices(const QString &system) const
{
    const QStringList configured = LogCfg::configuredDevices(system);
    if (configured.isEmpty())
        return {};

    const QStringList online = listAdbDevices();
    QStringList matched;
    for (const QString &serial : configured) {
        if (online.contains(serial))
            matched << serial;
    }
    return matched;
}
