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

// test_rt_model.hpp
struct TestItem {
    int    id;        // 内部序号，方便更新时定位
    QString name;     // 名称
    QString testCode; // 测试码
    QString status;   // 状态
    QString duration; // 时长
    QString message;  // 信息
    QString result;   // 结果
};

class RtModel : public QAbstractTableModel {
    Q_OBJECT
    DECLARE_SINGLETON(RtModel);
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
    Q_INVOKABLE void loadTestItems(const QVector<TestItem> &items);
    Q_INVOKABLE void updateTestValues(int row, const QString &status,const QString &duration,const QString &message,const QString &result);
private:
    QVector<TestItem> m_items;
};

#endif //FACTORYTESTMODULE_TEST_RT_MODEL_H
