//
// Created by panshiquan on 2026/5/12.
//

#ifndef FACTORYTESTMODULE_TEST_FLOW_CONTROLLER_H
#define FACTORYTESTMODULE_TEST_FLOW_CONTROLLER_H

#include <QObject>
#include <QHash>
#include <QTimer>
#include "test_rt_model.hpp"

// ---------- 测试流程控制器 ----------
class TestFlowController : public QObject {
    Q_OBJECT
public:
    explicit TestFlowController(RtModel *model, QObject *parent = nullptr)
        : QObject(parent), m_model(model) {
        Q_ASSERT(m_model);
    }

    // 加载计划并初始化界面状态（尚未开始计时）
    void loadTestPlan(const QVector<TestRunItem> &items) {
        if (m_running) stopAll();
        clearAll();

        m_model->loadTestItems(items);

        // 初始状态：未测试
        for (int row = 0; row < m_model->m_items.size(); ++row) {
            m_model->m_items[row].currentRetry = 0;
            m_model->m_items[row].active = false;
            m_model->updateTestValues(row, "waiting", "0s", "等待开始", "--");
        }
        m_model->set_total_num(QString::number(m_model->m_items.size()));
        m_model->set_pass_num("0");
        m_model->set_fail_num("0");
        m_model->set_untest_num(QString::number(m_model->m_items.size()));

        // 建立 id -> 行号映射
        m_idToRow.clear();
        for (int i = 0; i < m_model->m_items.size(); ++i) {
            m_idToRow[m_model->m_items[i].id] = i;
        }
    }

    // 立即启动所有测试项的定时器（由 TestManage 在开始测试时调用）
    void startAll() {
        if (m_running) return;
        if (m_model->m_items.isEmpty()) return;

        m_running = true;

        // 为每个测试项启动第一次超时等待
        for (int row = 0; row < m_model->m_items.size(); ++row) {
            startAttempt(row);
        }
    }

    // 停止（保留计划）
    void stopAll() {
        if (!m_running) return;
        m_running = false;

        // 停止并清理所有定时器
        for (auto *timer : m_timers) {
            timer->stop();
            timer->deleteLater();
        }
        m_timers.clear();

        // 将所有活跃项置为停止
        for (auto &item : m_model->m_items) {
            if (item.active) {
                item.active = false;
                int row = m_idToRow.value(item.id, -1);
                if (row >= 0)
                    m_model->updateTestValues(row, "stopped", "0s", "测试已停止", "STOP");
            }
        }
        emit allItemsFinished();
    }

    // 重新测试同一批计划（立即重新开始定时器）
    void restartAll() {
        if (m_running) stopAll();
        // 重置内部状态
        for (auto &item : m_model->m_items) {
            item.currentRetry = 0;
            item.active = false;
            int row = m_idToRow.value(item.id, -1);
            if (row >= 0)
                m_model->updateTestValues(row, "waiting", "0s", "等待开始", "--");
        }
        startAll();
    }

    // 完全复位（清空计划）
    void resetAll() {
        if (m_running) stopAll();
        clearAll();
        m_model->reset();   // 模型自身的复位
    }

public slots:
    // 收到测试结果（从 TestManage 跨线程调用）
    void onResponseReceived(int configId) {
        if (!m_running) return;

        int row = m_idToRow.value(configId, -1);
        if (row < 0) return;

        TestRunItem &item = m_model->m_items[row];
        if (!item.active) return;   // 已经超时或结束，忽略迟到的回复

        // 停止本项定时器
        stopTimerForItem(configId);
        item.active = false;

        // 成功
        m_model->updateTestValues(row, "pass", "1.5s", "测试通过", "PASS");
        emit itemFinished(configId, true);
        checkAllFinished();
    }

signals:
    void itemFinished(int configId, bool success);
    void allItemsFinished();

private:
    void startAttempt(int row) {
        TestRunItem &item = m_model->m_items[row];
        if (!m_running) return;

        item.active = true;

        // 创建超时定时器
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setProperty("configId", item.id);
        connect(timer, &QTimer::timeout, this, [this, id = item.id]() {
            onTimeout(id);
        });
        timer->start(item.timeoutMs);
        m_timers[item.id] = timer;

        // 更新界面：“正在测试”
        m_model->updateTestValues(row, "testing", "0s",
                                  QString("第 %1 次测试中...").arg(item.currentRetry + 1),
                                  "--");
    }

    void onTimeout(int configId) {
        if (!m_running) return;

        int row = m_idToRow.value(configId, -1);
        if (row < 0) return;

        TestRunItem &item = m_model->m_items[row];
        if (!item.active) return;

        stopTimerForItem(configId);
        item.active = false;

        if (item.currentRetry < item.maxRetries) {
            // 重试
            item.currentRetry++;
            startAttempt(row);
        } else {
            // 最终失败
            m_model->updateTestValues(row, "fail", "--", "超时未回复", "FAIL");
            emit itemFinished(configId, false);
            checkAllFinished();
        }
    }

    void checkAllFinished() {
        if (!m_running) return;
        for (const auto &item : m_model->m_items) {
            if (item.active) return;
        }
        m_running = false;
        emit allItemsFinished();
    }

    void stopTimerForItem(int configId) {
        if (m_timers.contains(configId)) {
            m_timers[configId]->stop();
            m_timers[configId]->deleteLater();
            m_timers.remove(configId);
        }
    }

    void clearAll() {
        for (auto *timer : m_timers)
            timer->deleteLater();
        m_timers.clear();
        m_idToRow.clear();
        m_running = false;
    }

    RtModel *m_model;
    QHash<int, QTimer *> m_timers;   // configId → 定时器
    QHash<int, int> m_idToRow;       // configId → 行号
    bool m_running = false;
};

#endif // FACTORYTESTMODULE_TEST_FLOW_CONTROLLER_H
