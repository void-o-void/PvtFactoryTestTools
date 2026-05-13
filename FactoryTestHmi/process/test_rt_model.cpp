//
// Created by panshiquan on 2026/5/9.
//
#include "test_rt_model.hpp"
#include <QRandomGenerator>


RtModel::RtModel(QObject *parent) : QAbstractTableModel(parent)
{
}

int RtModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

int RtModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 6; // id, name, standard, value, status, result
}

QVariant RtModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const TestRunItem &item = m_items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return item.name;
            case 1: return item.testCode;
            case 2: return item.status;
            case 3: return item.duration;
            case 4: return item.message;
            case 5: return item.result;
        }
    } else if (role == NameRole)     return item.name;
    else if (role == TestCodeRole)   return item.testCode;
    else if (role == StatusRole)     return item.status;
    else if (role == DurationRole)   return item.duration;
    else if (role == MessageRole)    return item.message;
    else if (role == ResultRole)     return item.result;

    // 前景色：结果列特殊处理
    if (role == Qt::ForegroundRole && index.column() == 5) {
        if (item.result == "pass") return QColor("#22c55e");
        if (item.result == "fail") return QColor("#ef4444");
    }
    return QVariant();
}

QHash<int, QByteArray> RtModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();
    roles[NameRole]     = "name";
    roles[TestCodeRole] = "testCode";
    roles[StatusRole]   = "status";
    roles[DurationRole] = "duration";
    roles[MessageRole]  = "message";
    roles[ResultRole]   = "result";
    return roles;
}

void RtModel::loadTestItems(const QVector<TestRunItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

void RtModel::updateTestValues(int row, const QString &status, const QString &duration, const QString &message, const QString &result) {
    if (row < 0 || row >= m_items.size()) return;

    m_items[row].status   = status;
    m_items[row].duration = duration;
    m_items[row].message  = message;
    m_items[row].result   = result;

    // 更新列 2~5
    QModelIndex topLeft = createIndex(row, 2);
    QModelIndex bottomRight = createIndex(row, 5);
    emit dataChanged(topLeft, bottomRight,{StatusRole, DurationRole, MessageRole, ResultRole});
}