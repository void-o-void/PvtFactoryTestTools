//
// Created by LEGION on 2026/4/18.
//
#include <QApplication>
#include <QPushButton>
#include "home.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    UiHome* home = new UiHome();
    home->resize(800, 500);
    home->show();
    return QApplication::exec();
}
