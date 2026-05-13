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

    // 加载计划（items 已带初始 idle/未开始，此处只设运行时字段）
    void loadTestPlan(const QVector<TestRunItem> &items) {
        if (m_running) stopAll();
        clearAll();

        m_model->loadTestItems(items);

        for (int row = 0; row < m_model->m_items.size(); ++row) {
            m_model->m_items[row].currentRetry = 0;
            m_model->m_items[row].active = false;
        }
        m_model->set_total_num(QString::number(m_model->m_items.size()));
        m_model->set_pass_num("0");
        m_model->set_fail_num("0");
        m_model->set_untest_num(QString::number(m_model->m_items.size()));

        // 建立 code → 行号映射
        m_codeToRow.clear();
        for (int i = 0; i < m_model->m_items.size(); ++i) {
            m_codeToRow[m_model->m_items[i].code] = i;
        }
    }

    // 点击开始后、握手前：所有项 → 待测试
    void setAllStandby() {
        for (int row = 0; row < m_model->m_items.size(); ++row) {
            m_model->updateTestValues(row, "standby", "--", "待测试", "--");
        }
    }

    // 立即启动所有测试项的定时器（通过 QueuedConnection 跨线程安全调用）
    Q_INVOKABLE void startAll() {
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
                int row = m_codeToRow.value(item.code, -1);
                if (row >= 0)
                    m_model->updateTestValues(row, "stopped", "--", "测试已停止", "--");
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
            int row = m_codeToRow.value(item.code, -1);
            if (row >= 0)
                m_model->updateTestValues(row, "waiting", "--", "等待开始", "--");
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
    // 收到测试结果（从 TestManage 跨线程调用，携带结果详情）
    void onItemResultReceived(int code, int state, const QString &msg) {
        if (!m_running) return;

        int row = m_codeToRow.value(code, -1);
        if (row < 0) return;

        TestRunItem &item = m_model->m_items[row];
        if (!item.active) return;

        stopTimerForItem(code);
        item.active = false;

        bool pass = (state == 1);
        QString result = pass ? "pass" : "fail";
        QString message = msg.isEmpty() ? (pass ? "测试通过" : "测试失败") : msg;

        m_model->updateTestValues(row, "finished", "--", message, result);
        emit itemFinished(code, pass);
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
        timer->setProperty("configId", item.code);
        connect(timer, &QTimer::timeout, this, [this, code = item.code]() {
            onTimeout(code);
        });
        timer->start(item.timeoutMs);
        m_timers[item.code] = timer;

        // 更新界面：QML 动画依赖 status == "processing"
        m_model->updateTestValues(row, "processing", "--",
                                  QString("第 %1 次测试中...").arg(item.currentRetry + 1),
                                  "--");
    }

    void onTimeout(int configId) {
        if (!m_running) return;

        int row = m_codeToRow.value(configId, -1);
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
            // 最终失败：status 用 "finished"，result 用 "fail"
            m_model->updateTestValues(row, "finished", "--", "超时未回复", "fail");
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
        m_codeToRow.clear();
        m_running = false;
    }

    RtModel *m_model;
    QHash<int, QTimer *> m_timers;   // code → 定时器
    QHash<int, int> m_codeToRow;     // code → 行号
    bool m_running = false;
};

#endif // FACTORYTESTMODULE_TEST_FLOW_CONTROLLER_H
