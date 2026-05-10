//
// Created by LEGION on 2026/4/18.
//
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "test_rt_model.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    RtModel *model = RtModel::instance();

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &warning: warnings)
            qWarning() << warning;
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [](const QUrl &url) {
                         qCritical() << "Failed to create object:" << url;
                     }, Qt::QueuedConnection);

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
                     [](const QList<QQmlError> &warnings) {
                         for (const auto &w: warnings)
                             qWarning() << w;
                     });

    engine.rootContext()->setContextProperty("rtModel", model);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("FactoryTestHmi", "Home");

    return QCoreApplication::exec();
}
