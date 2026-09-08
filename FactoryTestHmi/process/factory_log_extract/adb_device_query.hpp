#ifndef ADB_DEVICE_QUERY_HPP
#define ADB_DEVICE_QUERY_HPP

#include <QString>
#include <QStringList>

class LogExtractManage;

/**
 * ADB 设备查询：对外只暴露 getDevices。
 * getDevices(系统) = logcfg 中该系统配置型号 ∩ 当前 adb devices 已连接列表。
 * listAdbDevices 为私有，仅本类及友元 LogExtractManage 使用（保证拉取流程可用）。
 */
class AdbDeviceQuery {
public:
    AdbDeviceQuery() = default;

    /** 返回指定系统当前已连接、且在 logcfg 中配置过的型号列表 */
    QStringList getDevices(const QString &system) const;

    /** 解码 adb 进程输出（Windows 中文环境优先 GBK，避免乱码） */
    static QString decodeOutput(const QByteArray &raw);

private:
    friend class LogExtractManage;

    QStringList listAdbDevices() const;


    /** 返回 adb devices 中该 serial 的状态：device / offline / unauthorized / 空(未出现) */
    QString deviceStatus(const QString &serial) const;
    QString runAdb(const QStringList &args, int timeoutMs = 10000) const;
};

#endif
