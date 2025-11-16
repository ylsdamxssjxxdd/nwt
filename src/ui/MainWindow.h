#pragma once

#include "ShareCenterDialog.h"
#include "core/ChatController.h"

#include <QHash>
#include <QMainWindow>
#include <QPointer>

class QListView;
class ContactsSidebar;
class ChatPanel;
class PeerListModel;
class SettingsDialog;
class ProfileDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ChatController *controller, QWidget *parent = nullptr);

private slots:
    void handleSend();
    void handlePeerSelection(const QModelIndex &index);
    void appendMessage(const PeerInfo &peer, const QString &roleName, const QString &text);
    void showStatus(const QString &text);
    void openSettingsDialog();
    void openProfileDialog();
    void handleSendFile();
    void handleFileReceived(const PeerInfo &peer, const QString &roleName, const QString &fileName,
                            const QString &path);
    void handleShareCatalog(const QString &peerId, const QList<SharedFileInfo> &files);
    void openShareCenter();

private:
    void setupUi();
    void updatePeerPlaceholder();
    void refreshProfileCard();
    void updateChatHeader(const QString &displayName);

    ChatController *m_controller = nullptr;
    ContactsSidebar *m_contactsSidebar = nullptr;
    ChatPanel *m_chatPanel = nullptr;
    QListView *m_peerList = nullptr;
    PeerListModel *m_peerListModel = nullptr;
    QString m_currentPeerId;
    QPointer<SettingsDialog> m_settingsDialog;
    QPointer<ShareCenterDialog> m_shareDialog;
    QPointer<ProfileDialog> m_profileDialog;
    QHash<QString, QList<SharedFileInfo>> m_remoteShares;
};
