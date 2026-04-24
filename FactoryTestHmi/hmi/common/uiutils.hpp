//
// Created by LEGION on 2026/4/20.
//

#ifndef FACTORYTESTMODULE_UIUTILS_H
#define FACTORYTESTMODULE_UIUTILS_H
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

class UiUtils {
public:
    UiUtils() {}
    static QHBoxLayout* createEditBox(QWidget* parent, QWidget* &edit, QString title, float ratio1, QString text, float ratio2, QString type = "string", QStringList texts = QStringList()) {
        QHBoxLayout* layout = new QHBoxLayout;
        QLabel* label = new QLabel(title, parent);
        layout->addWidget(label, ratio1);

        if (type == "string") {
            QLineEdit* lineEdit = new QLineEdit(text , parent);
            lineEdit->setText(text);
            layout->addWidget(lineEdit, ratio2);
            edit = lineEdit;
        } else if (type == "combox") {
            QComboBox* combox = new QComboBox(parent);
            combox->addItems(texts);
            combox->setCurrentText(text);
            layout->addWidget(combox, ratio2);
            edit = combox;
        }
        return layout;
    }
};

#endif //FACTORYTESTMODULE_UIUTILS_H
