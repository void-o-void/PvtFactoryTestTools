//
// Created by panshiquan on 2026/5/9.
//
#include "test_rt_model.hpp"
#include <QRandomGenerator>

RtModel::RtModel(QObject *parent) : QAbstractTableModel(parent)
{
    // 生成 100 条假数据
    static const char *names[] = {
        "电池电压测试", "屏幕亮度测试", "WiFi 连接测试", "蓝牙配对测试",
        "扬声器测试", "麦克风测试", "摄像头测试", "触控测试",
        "陀螺仪测试", "加速度计测试", "GPS 定位测试", "NFC 读写测试",
        "充电测试", "USB 传输测试", "SD 卡测试", "按键耐久测试",
        "振动马达测试", "指纹识别测试", "人脸解锁测试", "温控测试"
    };
    static const char *testCodes[] = {
        "A1001", "A1002", "A1003", "A1004", "A1005",
        "B2001", "B2002", "B2003", "B2004", "B2005"
    };
    static const char *statusList[]  = { "waiting", "processing", "finished" };
    static const char *resultList[]  = { "pass", "fail" };
    static const char *durationTmpl = "%1.%2s";

    for (int i = 0; i < 100; ++i) {
        int st = (i < 85) ? 2           // 前 85 条已完成
               : (i < 95) ? 1           // 中间 10 条进行中
               : 0;                      // 最后 5 条等待中

        int res = st == 2 ? (i % 5 == 0 ? 1 : 0) : 0;  // 已完成中约 20% 失败
        int sec  = QRandomGenerator::global()->bounded(1, 30);
        int ms   = QRandomGenerator::global()->bounded(10, 99);

        TestItem item;
        item.id       = i + 1;
        item.name     = names[i % 20];
        item.testCode = testCodes[i % 10];
        item.status   = statusList[st];
        item.duration = (st == 2) ? QString(durationTmpl).arg(sec).arg(ms, 2, 10, QChar('0'))
                                  : "--";
        item.message  = (st == 2) ? (res == 0 ? "测试通过" : "电压超出范围")
                                  : "等待执行...";
        item.result   = (st == 2) ? resultList[res] : "--";

        m_items.append(item);
    }
}

int RtModel::rowCount(const QModelIndex &parent) const
{
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

    const TestItem &item = m_items.at(index.row());

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

void RtModel::loadTestItems(const QVector<TestItem> &items)
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