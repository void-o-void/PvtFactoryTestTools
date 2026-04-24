#ifndef FACTORYTESTMODULE_UICUSTOMDIALOG_H
#define FACTORYTESTMODULE_UICUSTOMDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

class UiCustomDialog : public QDialog {
    Q_OBJECT
public:
    explicit UiCustomDialog(QWidget* parent = nullptr) : QDialog(parent) {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        m_contentWidget = nullptr;
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        m_btnCancel = new QPushButton(tr("取消"), this);
        connect(m_btnCancel, &QPushButton::clicked, this, &UiCustomDialog::reject);
        buttonLayout->addWidget(m_btnCancel);

        m_btnOk = new QPushButton(tr("确定"), this);
        connect(m_btnOk, &QPushButton::clicked, this, &UiCustomDialog::accept);
        buttonLayout->addWidget(m_btnOk);

        mainLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
    }

    void setContentWidget(QWidget* widget, const QString& title) {
        setWindowTitle(title);

        if (!widget) return;

        QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
        if (!mainLayout) return;
        if (m_contentWidget) {
            mainLayout->removeWidget(m_contentWidget);
            m_contentWidget->deleteLater();
        }

        m_contentWidget = widget;
        mainLayout->insertWidget(0, m_contentWidget);
    }

    // 设置按钮文本（可选）
    void setOkButtonText(const QString& text) { m_btnOk->setText(text); }
    void setCancelButtonText(const QString& text) { m_btnCancel->setText(text); }

private:
    QWidget* m_contentWidget = nullptr;
    QPushButton* m_btnOk = nullptr;
    QPushButton* m_btnCancel = nullptr;
};

#endif // FACTORYTESTMODULE_UICUSTOMDIALOG_H