#pragma once

#include "DiscoveryService.h"
#include "MessageRouter.h"
#include "PeerDirectory.h"
#include "StorageManager.h"
#include "SettingsTypes.h"
#include "ShareManager.h"

#include <QHostAddress>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QVector>

class ChatController : public QObject {
    Q_OBJECT

public:
    explicit ChatController(QObject *parent = nullptr);
    ~ChatController() override;

    bool initialize();
    PeerDirectory *peerDirectory();
    QString localDisplayName() const;
    quint16 listenPort() const;
    QList<QPair<QHostAddress, int>> configuredSubnets() const;
    QList<QPair<QHostAddress, int>> blockedSubnets() const;
    const AppSettings &settings() const;
    QString signatureText() const { return m_settings.signatureText; }
    QVector<RoleProfile> availableRoles() const;
    RoleProfile activeRole() const;
    bool hasActiveRole() const;
    bool requiresRoleSelection() const { return !m_hasStoredRole; }
    ProfileDetails profileDetails() const;
    QVector<StoredMessage> recentMessages(const QString &peerId, int limit = 200);
    /*!
     * \brief recentPeerIds 返回最近有消息往来的联系人 ID 列表。
     * \param limit 最多返回的联系人数量上限
     * \return 最近联系人 ID 列表，按最近消息时间倒序排序
     */
    QVector<QString> recentPeerIds(int limit = 100) const;

public slots:
    void sendMessageToPeer(const QString &peerId, const QString &text);
    void sendFileToPeer(const QString &peerId, const QString &filePath);
    void requestPeerShareList(const QString &peerId);
    void requestPeerSharedFile(const QString &peerId, const QString &entryId);
    void shareCatalogToPeer(const QString &peerId);
    void addSubnet(const QHostAddress &network, int prefixLength);
    void setSubnets(const QList<QPair<QHostAddress, int>> &subnets);
    void setBlockedSubnets(const QList<QPair<QHostAddress, int>> &subnets);
    void setActiveRoleId(const QString &roleId);
    void updateGeneralSettings(const GeneralSettings &settings);
    void updateNetworkSettings(const NetworkSettings &settings);
    void updateNotificationSettings(const NotificationSettings &settings);
    void updateHotkeySettings(const HotkeySettings &settings);
    void updateSecuritySettings(const SecuritySettings &settings);
    void updateMailSettings(const MailSettings &settings);
    void updateSharedDirectories(const QStringList &directories);
    void updateSignatureText(const QString &signature);
    void updateProfileDetails(const ProfileDetails &details);

signals:
    void chatMessageReceived(const PeerInfo &peer, const QString &roleName, const QString &text);
    void fileReceived(const PeerInfo &peer, const QString &roleName, const QString &fileName, const QString &localPath);
    void shareCatalogReceived(const QString &peerId, const QList<SharedFileInfo> &files);
    void statusInfo(const QString &text);
    void controllerWarning(const QString &message);
    void preferencesChanged(const AppSettings &settings);
    void roleChanged(const RoleProfile &profile);
    void profileUpdated(const ProfileDetails &details);

private:
    QString dataDirectoryPath() const;
    QString databaseFilePath() const;
    void loadSettings();
    void loadKnownPeers();
    void recordChatHistory(const QString &peerId, const QString &roleName, const QString &content,
                           MessageDirection direction, const QString &messageType,
                           const QString &attachmentPath = QString());
    void persistSettings();
    PeerInfo findPeer(const QString &peerId) const;
    void initializeRoles();
    RoleProfile roleById(const QString &roleId) const;
    void handleRouterMessage(const PeerInfo &peer, const QJsonObject &payload);
    void handleFileMessage(const PeerInfo &peer, const QJsonObject &payload);
    void handleShareCatalog(const PeerInfo &peer, const QJsonObject &payload);
    void sendShareCatalogToPeer(const PeerInfo &peer);
    void handleShareRequest(const PeerInfo &peer, const QJsonObject &payload);
    void handleShareDownload(const PeerInfo &peer, const QJsonObject &payload);
    ProfileDetails parseProfileObject(const QJsonObject &object, const QString &nameFallback,
                                      const QString &signatureFallback) const;
    QJsonObject profileToJson(const ProfileDetails &details) const;
    PeerDirectory m_peerDirectory;
    DiscoveryService m_discovery;
    MessageRouter m_router;
    QString m_localId;
    QString m_displayName;
    quint16 m_listenPort = 45600;
    QList<QPair<QHostAddress, int>> m_subnets;
    QList<QPair<QHostAddress, int>> m_blockedSubnets;
    AppSettings m_settings;
    QVector<RoleProfile> m_roles;
    ShareManager m_shareManager;
    StorageManager m_storage;
    bool m_storageReady = false;
    bool m_hasStoredRole = false;
};
