//
// Created by LEGION on 2026/4/20.
//
#ifndef FACTORYTESTMODULE_UIFACTORY_TEST_SETTING_H
#define FACTORYTESTMODULE_UIFACTORY_TEST_SETTING_H
#include <QWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>

#include "uiutils.hpp"
#include "test_config.hpp"

class UiFactoryTestSetting : public QWidget {
    Q_OBJECT
public:
    explicit UiFactoryTestSetting(QWidget* parent = nullptr) : QWidget(parent) {
        load(Config::instance()->current_project);
    }

    void save() {

    }

    void load(const QString projectName) {
        QLayout* oldLayout = this->layout();
        if (oldLayout) {
            clearLayout(oldLayout);
            delete oldLayout;
        }

        m_envs_list.clear();
        m_items_list.clear();

        auto cfg = Config::readConfig(projectName);
        QHBoxLayout* project_layout = new QHBoxLayout();
        project_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_project_select, "项目选择", 2, cfg->project_Name, 4,
            "combox", QStringList() << "P39" << "P41" << "P4A"));
        project_layout->addStretch();

        QGridLayout* station_layout = new QGridLayout();
        station_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_user_no, "员工编号", 2, cfg->m_station_config.user_no, 4), 0,0);
        station_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_order_no, "工单号", 2, cfg->m_station_config.order_no, 4), 0,1);
        station_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_line_name, "线别", 2, cfg->m_station_config.line_name, 4), 1,0);
        station_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_station_name, "工序名称(站位)", 2, cfg->m_station_config.station_name, 4), 1,1);
        station_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_fixture_no, "夹具编号", 2, cfg->m_station_config.fixture_no, 4), 2,0);
        m_station_group = new QGroupBox("站位配置",this);
        m_station_group->setLayout(station_layout);


        QHBoxLayout* factory_layout = new QHBoxLayout();
        factory_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_factory_select, "工厂选择", 2, cfg->m_factory_config.factory_name, 4,
            "combox", QStringList() << "PVT01" << "PVT02"));
        factory_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_mes_ip, "mes ip", 2, cfg->m_factory_config.mes_ip, 4));
        factory_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_mes_port, "mes port", 2, QString::number(cfg->m_factory_config.mes_port), 4));
        factory_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)m_mes_update, "是否上传mes", 2, cfg->m_factory_config.mes_enable ? "上传" : "关闭", 4,
            "combox", QStringList() << "上传" << "关闭"));
        m_factory_group = new QGroupBox("工厂配置",this);
        m_factory_group->setLayout(factory_layout);

        QGridLayout* envs_layout = new QGridLayout();
        for (auto& env : cfg->m_env_items) {
            QLineEdit* env_line  = nullptr;
            envs_layout->addLayout(UiUtils::createEditBox(this, (QWidget*&)env_line, env.descr, 2, env.value, 4), env.sn / 2, env.sn % 2);
            env_line->setProperty("descr", QVariant::fromValue(env.descr));
            m_envs_list.append(env_line);
        }
        m_envs_group = new QGroupBox("测试环境配置",this);
        m_envs_group->setLayout(envs_layout);

        QGridLayout* items_layout = new QGridLayout();
        for (auto& item : cfg->m_test_items) {
            QCheckBox* item_check = new QCheckBox(item.descr, this);
            items_layout->addWidget(item_check, item.sn / 4, item.sn % 4);
            item_check->setProperty("code", QVariant::fromValue(item.code));
            m_items_list.append(item_check);
        }
        m_items_group = new QGroupBox("测试项目配置",this);
        m_items_group->setLayout(items_layout);

        QVBoxLayout* main_layout = new QVBoxLayout();
        main_layout->addLayout(project_layout);
        main_layout->addWidget(m_station_group);
        main_layout->addWidget(m_factory_group);
        main_layout->addWidget(m_envs_group);
        main_layout->addWidget(m_items_group);
        this->setLayout(main_layout);

        if (m_project_select) {
            disconnect(m_project_select, nullptr, this, nullptr);
            connect(m_project_select, &QComboBox::currentTextChanged,this, &UiFactoryTestSetting::onProjectChanged);
        }
    }

    void onProjectChanged(const QString& projectName) {
        load(projectName);
    }

    void clearLayout(QLayout* layout) {
        if (!layout) return;
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            } else if (item->layout()) {
                clearLayout(item->layout());
            }
            delete item;
        }
    }

private:
    QComboBox* m_project_select = nullptr;

    QGroupBox* m_station_group = nullptr;
    QLineEdit* m_user_no = nullptr;
    QLineEdit* m_order_no = nullptr;
    QLineEdit* m_line_name = nullptr;
    QLineEdit* m_station_name = nullptr;
    QLineEdit* m_fixture_no = nullptr;

    QGroupBox* m_factory_group = nullptr;
    QComboBox* m_factory_select = nullptr;
    QLineEdit* m_mes_ip = nullptr;
    QLineEdit* m_mes_port = nullptr;
    QComboBox* m_mes_update = nullptr;

    QGroupBox* m_connect_group = nullptr;
    QComboBox* m_connect_com = nullptr;
    QComboBox* m_connect_baud_rate = nullptr;
    QComboBox* m_debug_com = nullptr;
    QComboBox* m_debug_baud_rate = nullptr;

    QGroupBox* m_envs_group = nullptr;
    QList<QLineEdit*> m_envs_list;

    QGroupBox* m_items_group = nullptr;
    QList<QCheckBox*> m_items_list;
};

#endif //FACTORYTESTMODULE_UIFACTORY_TEST_SETTING_H
