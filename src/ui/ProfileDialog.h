#pragma once

#include "core/ChatController.h"

#include <QDialog>
#include <QLineEdit>
#include <QPointer>
#include <QComboBox>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVector>

/*!
 * \brief 个人资料对话框，支持查看与编辑当前用户信息。
 */
class ProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileDialog(ChatController *controller, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void enterEditMode();
    void saveProfile();

private:
    void buildViewMode();
    void buildEditMode();
    void applyProfileToFields(const ProfileDetails &profile);

    QPointer<ChatController> m_controller;
    QStackedWidget *m_stack = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QWidget *m_viewPanel = nullptr;
    QWidget *m_editPanel = nullptr;
    QVector<QLabel *> m_viewLabels;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_signatureEdit = nullptr;
    QComboBox *m_genderCombo = nullptr;
    QLineEdit *m_unitEdit = nullptr;
    QLineEdit *m_departmentEdit = nullptr;
    QLineEdit *m_phoneEdit = nullptr;
    QLineEdit *m_mobileEdit = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QLineEdit *m_versionEdit = nullptr;
    QLineEdit *m_ipEdit = nullptr;
};
