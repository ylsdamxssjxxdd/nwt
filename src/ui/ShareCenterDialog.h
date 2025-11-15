#pragma once

#include "core/ChatController.h"

#include <QDialog>
#include <QListWidget>
#include <QPointer>

/*!
 * \brief 文件共享中心，管理本地共享目录并查看远程共享列表。
 */
class ShareCenterDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShareCenterDialog(ChatController *controller, QWidget *parent = nullptr);

    /*!
     * \brief 指定当前会话的联系人，用于推送共享及请求下载。
     */
    void setPeerId(const QString &peerId);

    /*!
     * \brief 更新远端共享条目展示。
     */
    void setRemoteEntries(const QList<SharedFileInfo> &entries);

private slots:
    void addDirectory();
    void removeDirectory();
    void sendLocalShare();
    void requestRemoteShare();
    void downloadSelectedShare();

private:
    void refreshLocalDirectories();
    void refreshRemoteList();

    QPointer<ChatController> m_controller;
    QListWidget *m_localList = nullptr;
    QListWidget *m_remoteList = nullptr;
    QString m_peerId;
    QList<SharedFileInfo> m_remoteEntries;
};
