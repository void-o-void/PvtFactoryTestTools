//
// Created by LEGION on 2026/4/18.
//

#include "home.h"
#include "uicostom_dialog.hpp"

#include <QMetaType>
#include <QPushButton>
#include <QWidget>

UiHome::UiHome(QWidget *parent) : QWidget(parent) {
    QPushButton *btn=new QPushButton("clicked",this);
    connect(btn , &QPushButton::clicked, this, [this] {
        UiFactoryTestSetting* factory = new UiFactoryTestSetting();
        UiCustomDialog* dialog = new UiCustomDialog;
        dialog->setContentWidget(factory, "工厂测试设置");
        dialog->resize(800, 600);
        if(dialog->exec()==QDialog::Accepted) {

        }
    });
    btn->show();
}

UiHome::~UiHome() {

}
