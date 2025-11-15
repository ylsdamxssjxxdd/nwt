#pragma once

#include "core/ChatController.h"

#include <QButtonGroup>
#include <QDialog>
#include <QFrame>
#include <QPointer>
#include <QStackedWidget>

/*!
 * \brief 设置中心对话框，负责展示和修改平台偏好。
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(ChatController *controller, QWidget *parent = nullptr);

private slots:
    void switchCategory(int index);

private:
    void setupUi();
    void bindGeneralSettings(QWidget *section);
    void bindNetworkSettings(QWidget *section);
    void bindNotificationSettings(QWidget *section);
    void bindHotkeySettings(QWidget *section);
    void bindSecuritySettings(QWidget *section);
    void bindMailSettings(QWidget *section);
    QWidget *buildNavigation(QWidget *parent);
    QWidget *buildContentStack(QWidget *parent);
    QWidget *wrapScroll(QWidget *page);
    QFrame *createSection(const QString &title);

    QWidget *createGeneralPage();
    QWidget *createNetworkPage();
    QWidget *createNotificationPage();
    QWidget *createHotkeyPage();
    QWidget *createLockPage();
    QWidget *createMailPage();

    QPointer<ChatController> m_controller;
    QButtonGroup *m_navGroup = nullptr;
    QStackedWidget *m_stack = nullptr;
};
