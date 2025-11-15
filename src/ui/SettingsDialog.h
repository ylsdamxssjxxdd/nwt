#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QFrame>
#include <QStackedWidget>

/*!
 * \brief 设置中心对话框，提供多个分类的偏好控制。
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    /*!
     * \brief 构造设置对话框。
     * \param parent 父级窗口。
     */
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    /*!
     * \brief 切换导航分类。
     * \param index 目标页索引。
     */
    void switchCategory(int index);

private:
    void setupUi();
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

    QButtonGroup *m_navGroup = nullptr;
    QStackedWidget *m_stack = nullptr;
};
