//
// Created by panshiquan on 2026/5/19.
//

#include "mes_service.hpp"
#include "mes_hcs_manager.hpp"
#include "mes_dsxp_manager.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

MesService::MesService()
{
    m_nam = std::make_unique<QNetworkAccessManager>();
}

// ==================== init ====================
void MesService::init(const MesConfig& cfg)
{
    m_cfg     = cfg;
    m_manager = createManager(cfg, m_nam.get());
    m_ctx.clear();

    qDebug() << "[MesService] init factory=" << cfg.factoryName
             << "project=" << cfg.projectName
             << "debug=" << cfg.debug
             << "baseUrl=" << cfg.baseUrl();
}

// ==================== 工厂方法 ====================
std::unique_ptr<IMesManager> MesService::createManager(
    const MesConfig& cfg, QNetworkAccessManager* nam)
{
    if (cfg.debug) {
        // debug 模式：不上传 MES，返回空
        qDebug() << "[MesService] Debug mode, no MES manager created.";
        return nullptr;
    }

    if (cfg.isHcs()) {
        qDebug() << "[MesService] Creating HcsMesManager (PVT01)";
        return std::make_unique<HcsMesManager>(cfg, nam);
    }

    if (cfg.isDsxp()) {
        qDebug() << "[MesService] Creating DsxpMesManager (PVT02)";
        return std::make_unique<DsxpMesManager>(cfg, nam);
    }

    qDebug() << "[MesService] Unknown factory, no manager created.";
    return nullptr;
}

// ==================== checkStation ====================
void MesService::checkStation(const QString& sn, StationCallback callback)
{
    // debug 模式: 直接返回通过
    if (!m_manager) {
        qDebug() << "[MesService] Debug mode, checkStation skip. sn=" << sn;
        StationCheckResult r;
        r.passed = true;

        // debug 下也保存基础上下文（时间戳）
        m_ctx.sn        = sn;
        m_ctx.checkTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        callback(r);
        return;
    }

    StationCheckInput input;
    input.sn = sn;

    m_manager->checkStation(input, [this, sn, callback](const StationCheckResult& r) {
        if (r.passed) {
            // 保存过站上下文，供后续上传使用
            m_ctx.sn        = sn;
            m_ctx.checkTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            m_ctx.eventCode = r.eventCode;
            m_ctx.barcode   = r.barcode;
            m_ctx.segment   = r.segment;

            qDebug() << "[MesService] checkStation PASS. sn=" << sn
                     << "eventCode=" << m_ctx.eventCode
                     << "barcode=" << m_ctx.barcode;
        } else {
            m_ctx.clear();
            qDebug() << "[MesService] checkStation FAIL. sn=" << sn
                     << "msg=" << r.message;
        }
        callback(r);
    });
}

// ==================== saveResult ====================
void MesService::saveResult(const QString& sn,
                            bool testPassed,
                            const QString& failInfo,
                            const QVector<TestItemResult>& items,
                            UploadCallback callback)
{
    // debug 模式: 序列化请求体保存到本地文件
    if (!m_manager) {
        qDebug() << "[MesService] Debug mode, save to local file.";

        // 构建对应工厂的请求体 JSON 用于预览（这里同时构建两种格式）
        QJsonObject local;
        local["sn"]         = sn;
        local["testPassed"] = testPassed;
        local["failInfo"]   = failInfo;
        local["time"]       = m_ctx.checkTime;

        QJsonArray itemArr;
        for (const auto& item : items) {
            QJsonObject o;
            o["seq"]    = item.seq;
            o["name"]   = item.name;
            o["result"] = item.result;
            o["value"]  = item.value;
            itemArr.append(o);
        }
        local["items"] = itemArr;

        // 写文件
        QFile file(QDir::currentPath() + "/TestResult.json");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(QJsonDocument(local).toJson(QJsonDocument::Indented));
            file.close();
        }

        UploadResult r;
        r.success = true;
        r.message = "保存到本地 TestResult.json";
        callback(r);
        return;
    }

    // 正式模式: 通过 manager 上传
    SaveResultInput input;
    input.sn         = sn;
    input.testPassed = testPassed;
    input.failInfo   = failInfo;
    input.items      = items;

    // 注入过站上下文
    input.eventCode = m_ctx.eventCode;
    input.barcode   = m_ctx.barcode;
    input.checkTime = m_ctx.checkTime;

    m_manager->saveResult(input, [this, callback](const UploadResult& r) {
        if (r.success) {
            qDebug() << "[MesService] saveResult OK";
            m_ctx.clear();  // 上传成功后清除上下文
        } else {
            qDebug() << "[MesService] saveResult FAIL:" << r.message;
        }
        callback(r);
    });
}
