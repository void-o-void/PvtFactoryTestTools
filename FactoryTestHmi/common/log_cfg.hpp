/*
 * 原 log_cfg.hpp 已整份注释保留。
 * LogCfg 已合并至 process/factory_log_extract/log_extract_manage.hpp
 *
 * --- 以下为原内容 ---
 *
 * #ifndef FACTORYTESTMODULE_LOG_CFG_HPP
 * #define FACTORYTESTMODULE_LOG_CFG_HPP
 *
 * #include <QJsonObject>
 * #include <QString>
 * #include <QStringList>
 *
 * class LogCfg {
 * public:
 *     static bool load(QString *errMsg = nullptr);
 *     static bool reload(QString *errMsg = nullptr);
 *     static QString configPath();
 *     static QStringList systemNames();
 *     static QStringList getDevices(const QString &system);
 *     static QString adbPath();
 *     static QString remotePath();
 *     static QString localRoot();
 *     static int forwardRemotePort();
 * private:
 *     LogCfg() = delete;
 *     struct State {
 *         QJsonObject root;
 *         QString path;
 *         bool loaded = false;
 *     };
 *     static State &state();
 *     static QString resolveConfigPath();
 *     static QJsonObject defaultConfig();
 *     static QString normalizeSystemKey(const QString &system);
 *     static bool loadFromFile(const QString &path, QString *errMsg);
 * };
 *
 * #endif
 */
