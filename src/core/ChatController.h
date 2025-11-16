#pragma once

#include "DiscoveryService.h"
#include "MessageRouter.h"
#include "PeerDirectory.h"

#include <QHostAddress>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QMetaType>
#include <QStringList>
#include <QVector>

/*!
 * \brief ���涨ɫ����Ϣ�����������峣.
 */
struct RoleProfile {
    QString id;
    QString name;
    QString signature;
    QString avatarLetter;
};

/*!
 * \brief �ļ������е�������Ϣ.
 */
struct SharedFileInfo {
    QString entryId;
    QString name;
    quint64 size = 0;
    QString filePath;
};
Q_DECLARE_METATYPE(SharedFileInfo)

/*!
 * \brief 个人资料信息。
 */
struct ProfileDetails {
    QString name;
    QString signature;
    QString gender;
    QString unit;
    QString department;
    QString phone;
    QString mobile;
    QString email;
    QString version;
    QString ip;
};

/*!
 * \brief ������ͨ�������������õ���һ��������
 */
struct GeneralSettings {
    bool autoStart = false;
    bool enableLanUpdate = true;
    bool rejectShake = false;
    bool rejectFeigeAds = false;
    bool enableFeigeGroup = true;
    bool autoMinimize = false;
    bool hideIpAddress = false;
    bool rejectSegmentShare = false;
    bool enableAutoGroup = true;
    bool enableAutoSubGroup = false;
    bool popupOnMessage = false;
    bool flashOnMessage = true;
    QString closeBehavior = QStringLiteral("taskbar");
    QString friendDisplay = QStringLiteral("signature");
};

/*!
 * \brief �����������.
 */
struct NetworkSettings {
    quint16 searchPort = 9011;
    QString organizationCode;
    bool enableInterop = false;
    quint16 interopPort = 2425;
    bool bindNetworkInterface = false;
    QString boundInterfaceId;
    bool restrictToListedSubnets = false;
    bool autoRefresh = true;
    int refreshIntervalMinutes = 5;
};

/*!
 * \brief ֪ͨ���ù���.
 */
struct NotificationSettings {
    bool notifySelfOnline = false;
    bool notifyPeerOnline = false;
    bool notifyIncomingMessage = true;
    bool notifyFileTransfer = true;
    bool notifyMailChange = false;
    bool muteAll = false;
    bool playGroupSound = false;
};

/*!
 * \brief ���������á�
 */
struct HotkeySettings {
    bool toggleMainWindow = true;
    QString toggleShortcut = QStringLiteral("Ctrl + I");
    bool captureEnabled = true;
    QString captureShortcut = QStringLiteral("Ctrl + Alt + X");
    bool lockEnabled = true;
    QString lockShortcut = QStringLiteral("Ctrl + L");
    bool openStorageEnabled = true;
    QString openStorageShortcut = QStringLiteral("Ctrl + R");
};

/*!
 * \brief �ð�ȫ����.
 */
struct SecuritySettings {
    enum IdleAction { SwitchAway, LockApplication };

    bool lockEnabled = false;
    QString passwordHash;
    int idleMinutes = 5;
    IdleAction idleAction = SwitchAway;
};

/*!
 * \brief �ʼ����á�
 */
struct MailSettings {
    QString address;
    QString imapServer;
    QString smtpServer;
    int port = 465;
    bool playSoundOnMail = false;
    bool popupOnMail = false;
};

/*!
 * \brief ȫ�����������õ�һ����������.
 */
struct AppSettings {
    GeneralSettings general;
    NetworkSettings network;
    NotificationSettings notifications;
    HotkeySettings hotkeys;
    SecuritySettings security;
    MailSettings mail;
    QStringList sharedDirectories;
    QString activeRoleId;
    QString signatureText;
    ProfileDetails profile;
};

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
    const AppSettings &settings() const;
    QString signatureText() const { return m_settings.signatureText; }
    QVector<RoleProfile> availableRoles() const;
    RoleProfile activeRole() const;
    bool hasActiveRole() const;
    bool requiresRoleSelection() const { return !m_hasStoredRole; }
    ProfileDetails profileDetails() const;

public slots:
    void sendMessageToPeer(const QString &peerId, const QString &text);
    void sendFileToPeer(const QString &peerId, const QString &filePath);
    void requestPeerShareList(const QString &peerId);
    void requestPeerSharedFile(const QString &peerId, const QString &entryId);
    void shareCatalogToPeer(const QString &peerId);
    void addSubnet(const QHostAddress &network, int prefixLength);
    void setSubnets(const QList<QPair<QHostAddress, int>> &subnets);
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
    QString configFilePath() const;
    void loadSettings();
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
    QList<SharedFileInfo> collectLocalShares();
    QString saveIncomingFile(const QString &fileName, const QByteArray &data);
    QString buildShareEntryId(const QString &filePath) const;
    ProfileDetails parseProfileObject(const QJsonObject &object) const;
    QJsonObject profileToJson(const ProfileDetails &details) const;

    PeerDirectory m_peerDirectory;
    DiscoveryService m_discovery;
    MessageRouter m_router;
    QString m_localId;
    QString m_displayName;
    quint16 m_listenPort = 45600;
    QList<QPair<QHostAddress, int>> m_subnets;
    AppSettings m_settings;
    QVector<RoleProfile> m_roles;
    QHash<QString, SharedFileInfo> m_localShareIndex;
    QHash<QString, QList<SharedFileInfo>> m_remoteShareCatalogs;
    bool m_hasStoredRole = false;
};
