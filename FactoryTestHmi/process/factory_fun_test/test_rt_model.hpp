//
// Created by panshiquan on 2026/5/9.
//

#ifndef FACTORYTESTMODULE_TEST_RT_MODEL_H
#define FACTORYTESTMODULE_TEST_RT_MODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QHash>
#include <QByteArray>
#include <QColor>

#include "common.hpp"
#include "uicommon.hpp"
struct TestRunItem {
    int    code = 0;        // 测试项编码，唯一标识（对应 TestConfigItem::code）
    QString name;
    QString testCode;
    QString status;
    QString duration;
    QString message;
    QString result;

    // 运行时控制字段（不暴露给 QML）
    int    timeoutMs     = 5000;
    int    maxRetries    = 0;
    int    currentRetry  = 0;
    bool   active        = false;
};

class RtModel : public QAbstractTableModel {
    friend class TestFlowController;
    Q_OBJECT
    DECLARE_SINGLETON(RtModel);

    AUTO_PROPERTY(QString, total_num);
    AUTO_PROPERTY(QString, pass_num);
    AUTO_PROPERTY(QString, fail_num);
    AUTO_PROPERTY(QString, untest_num);

    AUTO_PROPERTY(int, state);
    AUTO_PROPERTY(QString, duration);

    

    explicit RtModel(QObject *parent = nullptr);

public:
    enum TestRoles {
        NameRole = Qt::UserRole + 1,
        TestCodeRole,
        StatusRole,
        DurationRole,
        MessageRole,
        ResultRole
    };

    // 必须实现的纯虚函数
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // 业务接口
    Q_INVOKABLE void loadTestItems(const QVector<TestRunItem> &items);
    Q_INVOKABLE void updateTestValues(int row, const QString &status,const QString &duration,const QString &message,const QString &result);

    void reset() {
        set_state(1);
        set_total_num(QString::number(m_items.size()));
        set_pass_num(QString::number(0));
        set_fail_num(QString::number(0));
        set_untest_num(QString::number(m_items.size()));

        for (int row = 0; row < m_items.size(); row++) {
            updateTestValues(row, "waiting", "0s", "等待测试", "--");
        }
    }
private:
    QVector<TestRunItem> m_items;
};

#endif //FACTORYTESTMODULE_TEST_RT_MODEL_H
