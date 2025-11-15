#pragma once

#include "core/ChatController.h"

#include <QDialog>
#include <QListWidget>
#include <QPointer>

/*!
 * \brief 角色选择对话框，启动及会话过程中可切换身份。
 */
class RoleSelectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoleSelectionDialog(ChatController *controller, QWidget *parent = nullptr);

private slots:
    void handleAccept();

private:
    void populateRoles();

    QPointer<ChatController> m_controller;
    QListWidget *m_roleList = nullptr;
    QLineEdit *m_signatureEdit = nullptr;
};
