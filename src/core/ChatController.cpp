#include "ChatController.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>
#include <limits>

namespace {
bool jsonBool(const QJsonObject &object, const QString &key, bool fallback) {
    return object.contains(key) ? object.value(key).toBool(fallback) : fallback;
}

int jsonInt(const QJsonObject &object, const QString &key, int fallback) {
    return object.contains(key) ? object.value(key).toInt(fallback) : fallback;
}

GeneralSettings parseGeneral(const QJsonObject &object) {
    GeneralSettings settings;
    settings.autoStart = jsonBool(object, QStringLiteral("autoStart"), settings.autoStart);
    settings.enableLanUpdate = jsonBool(object, QStringLiteral("lanUpdate"), settings.enableLanUpdate);
    settings.rejectShake = jsonBool(object, QStringLiteral("rejectShake"), settings.rejectShake);
    settings.rejectFeigeAds = jsonBool(object, QStringLiteral("rejectFeigeAds"), settings.rejectFeigeAds);
    settings.enableFeigeGroup = jsonBool(object, QStringLiteral("enableFeigeGroup"), settings.enableFeigeGroup);
    settings.autoMinimize = jsonBool(object, QStringLiteral("autoMinimize"), settings.autoMinimize);
    settings.hideIpAddress = jsonBool(object, QStringLiteral("hideIpAddress"), settings.hideIpAddress);
    settings.rejectSegmentShare = jsonBool(object, QStringLiteral("rejectSegmentShare"), settings.rejectSegmentShare);
    settings.enableAutoGroup = jsonBool(object, QStringLiteral("autoGroup"), settings.enableAutoGroup);
    settings.enableAutoSubGroup = jsonBool(object, QStringLiteral("autoSubGroup"), settings.enableAutoSubGroup);
    settings.popupOnMessage = jsonBool(object, QStringLiteral("popupOnMessage"), settings.popupOnMessage);
    settings.flashOnMessage = jsonBool(object, QStringLiteral("flashOnMessage"), settings.flashOnMessage);
    settings.closeBehavior = object.value(QStringLiteral("closeBehavior")).toString(settings.closeBehavior);
    settings.friendDisplay = object.value(QStringLiteral("friendDisplay")).toString(settings.friendDisplay);
    return settings;
}

QJsonObject generalToJson(const GeneralSettings &settings) {
    return {
        {QStringLiteral("autoStart"), settings.autoStart},
        {QStringLiteral("lanUpdate"), settings.enableLanUpdate},
        {QStringLiteral("rejectShake"), settings.rejectShake},
        {QStringLiteral("rejectFeigeAds"), settings.rejectFeigeAds},
        {QStringLiteral("enableFeigeGroup"), settings.enableFeigeGroup},
        {QStringLiteral("autoMinimize"), settings.autoMinimize},
        {QStringLiteral("hideIpAddress"), settings.hideIpAddress},
        {QStringLiteral("rejectSegmentShare"), settings.rejectSegmentShare},
        {QStringLiteral("autoGroup"), settings.enableAutoGroup},
        {QStringLiteral("autoSubGroup"), settings.enableAutoSubGroup},
        {QStringLiteral("popupOnMessage"), settings.popupOnMessage},
        {QStringLiteral("flashOnMessage"), settings.flashOnMessage},
        {QStringLiteral("closeBehavior"), settings.closeBehavior},
        {QStringLiteral("friendDisplay"), settings.friendDisplay}
    };
}

NetworkSettings parseNetworkSettings(const QJsonObject &object) {
    NetworkSettings settings;
    settings.autoRefresh = jsonBool(object, QStringLiteral("autoRefresh"), settings.autoRefresh);
    settings.refreshIntervalMinutes = jsonInt(object, QStringLiteral("refreshInterval"), settings.refreshIntervalMinutes);
    return settings;
}

QJsonObject networkSettingsToJson(const NetworkSettings &settings) {
    return {
        {QStringLiteral("autoRefresh"), settings.autoRefresh},
        {QStringLiteral("refreshInterval"), settings.refreshIntervalMinutes}
    };
}

NotificationSettings parseNotificationSettings(const QJsonObject &object) {
    NotificationSettings settings;
    settings.notifySelfOnline = jsonBool(object, QStringLiteral("notifySelfOnline"), settings.notifySelfOnline);
    settings.notifyPeerOnline = jsonBool(object, QStringLiteral("notifyPeerOnline"), settings.notifyPeerOnline);
    settings.notifyIncomingMessage =
        jsonBool(object, QStringLiteral("notifyIncomingMessage"), settings.notifyIncomingMessage);
    settings.notifyFileTransfer = jsonBool(object, QStringLiteral("notifyFileTransfer"), settings.notifyFileTransfer);
    settings.notifyMailChange = jsonBool(object, QStringLiteral("notifyMailChange"), settings.notifyMailChange);
    settings.muteAll = jsonBool(object, QStringLiteral("muteAll"), settings.muteAll);
    settings.playGroupSound = jsonBool(object, QStringLiteral("playGroupSound"), settings.playGroupSound);
    return settings;
}

QJsonObject notificationSettingsToJson(const NotificationSettings &settings) {
    return {
        {QStringLiteral("notifySelfOnline"), settings.notifySelfOnline},
        {QStringLiteral("notifyPeerOnline"), settings.notifyPeerOnline},
        {QStringLiteral("notifyIncomingMessage"), settings.notifyIncomingMessage},
        {QStringLiteral("notifyFileTransfer"), settings.notifyFileTransfer},
        {QStringLiteral("notifyMailChange"), settings.notifyMailChange},
        {QStringLiteral("muteAll"), settings.muteAll},
        {QStringLiteral("playGroupSound"), settings.playGroupSound}
    };
}

HotkeySettings parseHotkeySettings(const QJsonObject &object) {
    HotkeySettings settings;
    settings.toggleMainWindow =
        jsonBool(object, QStringLiteral("toggleMainWindow"), settings.toggleMainWindow);
    settings.toggleShortcut = object.value(QStringLiteral("toggleShortcut")).toString(settings.toggleShortcut);
    settings.captureEnabled = jsonBool(object, QStringLiteral("captureEnabled"), settings.captureEnabled);
    settings.captureShortcut = object.value(QStringLiteral("captureShortcut")).toString(settings.captureShortcut);
    settings.lockEnabled = jsonBool(object, QStringLiteral("lockEnabled"), settings.lockEnabled);
    settings.lockShortcut = object.value(QStringLiteral("lockShortcut")).toString(settings.lockShortcut);
    settings.openStorageEnabled =
        jsonBool(object, QStringLiteral("openStorageEnabled"), settings.openStorageEnabled);
    settings.openStorageShortcut =
        object.value(QStringLiteral("openStorageShortcut")).toString(settings.openStorageShortcut);
    return settings;
}

QJsonObject hotkeySettingsToJson(const HotkeySettings &settings) {
    return {
        {QStringLiteral("toggleMainWindow"), settings.toggleMainWindow},
        {QStringLiteral("toggleShortcut"), settings.toggleShortcut},
        {QStringLiteral("captureEnabled"), settings.captureEnabled},
        {QStringLiteral("captureShortcut"), settings.captureShortcut},
        {QStringLiteral("lockEnabled"), settings.lockEnabled},
        {QStringLiteral("lockShortcut"), settings.lockShortcut},
        {QStringLiteral("openStorageEnabled"), settings.openStorageEnabled},
        {QStringLiteral("openStorageShortcut"), settings.openStorageShortcut}
    };
}

SecuritySettings parseSecuritySettings(const QJsonObject &object) {
    SecuritySettings settings;
    settings.lockEnabled = jsonBool(object, QStringLiteral("lockEnabled"), settings.lockEnabled);
    settings.passwordHash = object.value(QStringLiteral("passwordHash")).toString(settings.passwordHash);
    settings.idleMinutes = jsonInt(object, QStringLiteral("idleMinutes"), settings.idleMinutes);
    const QString idleMode = object.value(QStringLiteral("idleAction")).toString(QStringLiteral("away"));
    settings.idleAction =
        idleMode == QStringLiteral("lock") ? SecuritySettings::LockApplication : SecuritySettings::SwitchAway;
    return settings;
}

QJsonObject securitySettingsToJson(const SecuritySettings &settings) {
    return {
        {QStringLiteral("lockEnabled"), settings.lockEnabled},
        {QStringLiteral("passwordHash"), settings.passwordHash},
        {QStringLiteral("idleMinutes"), settings.idleMinutes},
        {QStringLiteral("idleAction"),
         settings.idleAction == SecuritySettings::LockApplication ? QStringLiteral("lock")
                                                                  : QStringLiteral("away")}
    };
}

MailSettings parseMailSettings(const QJsonObject &object) {
    MailSettings settings;
    settings.address = object.value(QStringLiteral("address")).toString(settings.address);
    settings.imapServer = object.value(QStringLiteral("imapServer")).toString(settings.imapServer);
    settings.smtpServer = object.value(QStringLiteral("smtpServer")).toString(settings.smtpServer);
    settings.port = jsonInt(object, QStringLiteral("port"), settings.port);
    settings.playSoundOnMail = jsonBool(object, QStringLiteral("playSound"), settings.playSoundOnMail);
    settings.popupOnMail = jsonBool(object, QStringLiteral("popupOnMail"), settings.popupOnMail);
    return settings;
}

QJsonObject mailSettingsToJson(const MailSettings &settings) {
    return {
        {QStringLiteral("address"), settings.address},
        {QStringLiteral("imapServer"), settings.imapServer},
        {QStringLiteral("smtpServer"), settings.smtpServer},
        {QStringLiteral("port"), settings.port},
        {QStringLiteral("playSound"), settings.playSoundOnMail},
        {QStringLiteral("popupOnMail"), settings.popupOnMail}
    };
}

QStringList parseStringList(const QJsonArray &array) {
    QStringList result;
    for (const QJsonValue &value : array) {
        result.append(value.toString());
    }
    return result;
}

QJsonArray toJsonArray(const QStringList &list) {
    QJsonArray array;
    for (const QString &value : list) {
        array.append(value);
    }
    return array;
}
} // namespace

ChatController::ChatController(QObject *parent) : QObject(parent) {
    qRegisterMetaType<PeerInfo>("PeerInfo");
    qRegisterMetaType<QList<SharedFileInfo>>("QList<SharedFileInfo>");

    connect(&m_discovery, &DiscoveryService::peerDiscovered, &m_peerDirectory, &PeerDirectory::upsertPeer);
    connect(&m_discovery, &DiscoveryService::discoveryWarning, this, &ChatController::controllerWarning);
    connect(&m_router, &MessageRouter::routerWarning, this, &ChatController::controllerWarning);
    connect(&m_router, &MessageRouter::messageReceived, this, &ChatController::handleRouterMessage);
}

bool ChatController::initialize() {
    loadSettings();
    initializeRoles();
    if (m_settings.activeRoleId.isEmpty() && !m_roles.isEmpty()) {
        m_settings.activeRoleId = m_roles.front().id;
    }

    if (!m_router.startListening(m_listenPort)) {
        emit controllerWarning(tr("消息路由启动失败"));
    }
    m_router.setLocalPeerId(m_localId);
    m_router.setLocalDisplayName(m_displayName);

    m_discovery.setLocalIdentity(m_localId, m_displayName, m_listenPort);
    m_discovery.setSubnets(m_subnets);
    m_discovery.start();
    m_discovery.announceOnline();

    emit statusInfo(tr("启动完成，ID: %1 端口: %2").arg(m_localId).arg(m_listenPort));
    persistSettings();
    return true;
}

PeerDirectory *ChatController::peerDirectory() {
    return &m_peerDirectory;
}

QString ChatController::localDisplayName() const {
    return m_displayName;
}

quint16 ChatController::listenPort() const {
    return m_listenPort;
}

QList<QPair<QHostAddress, int>> ChatController::configuredSubnets() const {
    return m_subnets;
}

const AppSettings &ChatController::settings() const {
    return m_settings;
}

QVector<RoleProfile> ChatController::availableRoles() const {
    return m_roles;
}

RoleProfile ChatController::activeRole() const {
    return roleById(m_settings.activeRoleId);
}

bool ChatController::hasActiveRole() const {
    return !m_settings.activeRoleId.isEmpty();
}

void ChatController::sendMessageToPeer(const QString &peerId, const QString &text) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(tr("未找到联系人 %1").arg(peerId));
        return;
    }
    const RoleProfile profile = activeRole();
    const QString roleName = profile.id.isEmpty() ? m_displayName : profile.name;
    m_router.sendChatMessage(peer, text, profile.id, roleName);
}

void ChatController::sendFileToPeer(const QString &peerId, const QString &filePath) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(tr("未找到联系人 %1").arg(peerId));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit controllerWarning(tr("无法读取文件: %1").arg(file.errorString()));
        return;
    }

    const QByteArray data = file.readAll();
    QJsonObject payload{
        {QStringLiteral("fileName"), QFileInfo(file).fileName()},
        {QStringLiteral("fileSize"), static_cast<qint64>(data.size())},
        {QStringLiteral("data"), QString::fromLatin1(data.toBase64())}
    };
    const RoleProfile profile = activeRole();
    const QString roleName = profile.id.isEmpty() ? m_displayName : profile.name;
    m_router.sendFilePayload(peer, profile.id, roleName, payload);
    emit statusInfo(tr("已发送文件 %1").arg(file.fileName()));
}

void ChatController::requestPeerShareList(const QString &peerId) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(tr("未找到联系人 %1").arg(peerId));
        return;
    }
    QJsonObject payload{{QStringLiteral("type"), QStringLiteral("share_request")}};
    m_router.sendSharePayload(peer, payload);
}

void ChatController::requestPeerSharedFile(const QString &peerId, const QString &entryId) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(tr("未找到联系人 %1").arg(peerId));
        return;
    }
    QJsonObject payload{
        {QStringLiteral("type"), QStringLiteral("share_download")},
        {QStringLiteral("entryId"), entryId}
    };
    m_router.sendSharePayload(peer, payload);
}

void ChatController::shareCatalogToPeer(const QString &peerId) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(tr("未找到联系人 %1").arg(peerId));
        return;
    }
    sendShareCatalogToPeer(peer);
}

void ChatController::addSubnet(const QHostAddress &network, int prefixLength) {
    const QString token = QStringLiteral("%1/%2").arg(network.toString()).arg(prefixLength);
    for (const auto &pair : std::as_const(m_subnets)) {
        const QString existing = QStringLiteral("%1/%2").arg(pair.first.toString()).arg(pair.second);
        if (existing == token) {
            return;
        }
    }
    m_subnets.append(qMakePair(network, prefixLength));
    m_discovery.setSubnets(m_subnets);
    m_discovery.probeSubnet(network, prefixLength);
    m_discovery.announceOnline();
    persistSettings();
}

void ChatController::setActiveRoleId(const QString &roleId) {
    if (m_settings.activeRoleId == roleId) {
        return;
    }
    m_settings.activeRoleId = roleId;
    persistSettings();
    emit roleChanged(activeRole());
}

void ChatController::updateGeneralSettings(const GeneralSettings &settings) {
    m_settings.general = settings;
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateNetworkSettings(const NetworkSettings &settings) {
    m_settings.network = settings;
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateNotificationSettings(const NotificationSettings &settings) {
    m_settings.notifications = settings;
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateHotkeySettings(const HotkeySettings &settings) {
    m_settings.hotkeys = settings;
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateSecuritySettings(const SecuritySettings &settings) {
    m_settings.security = settings;
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateMailSettings(const MailSettings &settings) {
    m_settings.mail = settings;
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateSharedDirectories(const QStringList &directories) {
    m_settings.sharedDirectories = directories;
    collectLocalShares();
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateSignatureText(const QString &signature) {
    m_settings.signatureText = signature;
    persistSettings();
    emit preferencesChanged(m_settings);
}

QString ChatController::configFilePath() const {
    const QString baseDir = QCoreApplication::applicationDirPath();
    return QDir(baseDir).filePath(QStringLiteral("lan_chat_settings.json"));
}

void ChatController::loadSettings() {
    QFile file(configFilePath());
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const QJsonObject root = doc.object();
        const QJsonObject identity = root.value(QStringLiteral("identity")).toObject();
        const QJsonObject network = root.value(QStringLiteral("network")).toObject();
        const QJsonObject preferences = root.value(QStringLiteral("preferences")).toObject();

        m_localId = identity.value(QStringLiteral("id")).toString();
        m_displayName = identity.value(QStringLiteral("name")).toString();
        if (m_displayName.isEmpty()) {
            m_displayName = m_localId;
        }
        const int portValue = network.value(QStringLiteral("listenPort")).toInt(m_listenPort);
        if (portValue > 0 && portValue <= std::numeric_limits<quint16>::max()) {
            m_listenPort = static_cast<quint16>(portValue);
        }

        const QJsonArray subnetArray = network.value(QStringLiteral("subnets")).toArray();
        m_subnets.clear();
        for (const QJsonValue &value : subnetArray) {
            const QJsonObject entry = value.toObject();
            QHostAddress addr(entry.value(QStringLiteral("network")).toString());
            const int prefix = entry.value(QStringLiteral("prefix")).toInt(-1);
            if (!addr.isNull() && prefix >= 0 && prefix <= 32) {
                m_subnets.append(qMakePair(addr, prefix));
            }
        }

        m_settings.general = parseGeneral(preferences.value(QStringLiteral("general")).toObject());
        m_settings.network = parseNetworkSettings(preferences.value(QStringLiteral("networkPrefs")).toObject());
        m_settings.notifications =
            parseNotificationSettings(preferences.value(QStringLiteral("notifications")).toObject());
        m_settings.hotkeys = parseHotkeySettings(preferences.value(QStringLiteral("hotkeys")).toObject());
        m_settings.security = parseSecuritySettings(preferences.value(QStringLiteral("security")).toObject());
        m_settings.mail = parseMailSettings(preferences.value(QStringLiteral("mail")).toObject());
        m_settings.sharedDirectories =
            parseStringList(preferences.value(QStringLiteral("sharedFolders")).toArray());
        m_settings.activeRoleId = preferences.value(QStringLiteral("activeRoleId")).toString();
        m_hasStoredRole = !m_settings.activeRoleId.isEmpty();
        m_settings.signatureText =
            preferences.value(QStringLiteral("signature")).toString(QStringLiteral("编辑个性签名"));
    }

    if (m_localId.isEmpty()) {
        m_localId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (m_displayName.isEmpty()) {
        m_displayName = QStringLiteral("EVA-0");
    }
    if (m_settings.signatureText.isEmpty()) {
        m_settings.signatureText = QStringLiteral("编辑个性签名");
    }
    collectLocalShares();
}

void ChatController::persistSettings() {
    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit controllerWarning(tr("无法写入配置: %1").arg(file.errorString()));
        return;
    }

    QJsonObject identity{
        {QStringLiteral("id"), m_localId},
        {QStringLiteral("name"), m_displayName}
    };

    QJsonArray subnetArray;
    for (const auto &pair : std::as_const(m_subnets)) {
        subnetArray.append(QJsonObject{
            {QStringLiteral("network"), pair.first.toString()},
            {QStringLiteral("prefix"), pair.second}
        });
    }

    QJsonObject network{
        {QStringLiteral("listenPort"), static_cast<int>(m_listenPort)},
        {QStringLiteral("subnets"), subnetArray}
    };

    QJsonObject preferences{
        {QStringLiteral("general"), generalToJson(m_settings.general)},
        {QStringLiteral("networkPrefs"), networkSettingsToJson(m_settings.network)},
        {QStringLiteral("notifications"), notificationSettingsToJson(m_settings.notifications)},
        {QStringLiteral("hotkeys"), hotkeySettingsToJson(m_settings.hotkeys)},
        {QStringLiteral("security"), securitySettingsToJson(m_settings.security)},
        {QStringLiteral("mail"), mailSettingsToJson(m_settings.mail)},
        {QStringLiteral("sharedFolders"), toJsonArray(m_settings.sharedDirectories)},
        {QStringLiteral("activeRoleId"), m_settings.activeRoleId},
        {QStringLiteral("signature"), m_settings.signatureText}
    };

    QJsonObject root{
        {QStringLiteral("identity"), identity},
        {QStringLiteral("network"), network},
        {QStringLiteral("preferences"), preferences}
    };

    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

PeerInfo ChatController::findPeer(const QString &peerId) const {
    for (const auto &peer : m_peerDirectory.peers()) {
        if (peer.id == peerId) {
            return peer;
        }
    }
    return {};
}

void ChatController::initializeRoles() {
    if (!m_roles.isEmpty()) {
        return;
    }
    m_roles = {
        RoleProfile{QStringLiteral("scout"), QStringLiteral("EVA-0"), QStringLiteral("守护通道的前哨"), QStringLiteral("E")},
        RoleProfile{QStringLiteral("analyst"), QStringLiteral("IDEA-1"), QStringLiteral("洞察数据的策士"), QStringLiteral("I")},
        RoleProfile{QStringLiteral("guardian"), QStringLiteral("Aegis"), QStringLiteral("稳健可靠的守护者"), QStringLiteral("A")}
    };
}

RoleProfile ChatController::roleById(const QString &roleId) const {
    for (const RoleProfile &profile : m_roles) {
        if (profile.id == roleId) {
            return profile;
        }
    }
    return RoleProfile{};
}

void ChatController::handleRouterMessage(const PeerInfo &peer, const QJsonObject &payload) {
    const QString type = payload.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("chat")) {
        const QString roleName = payload.value(QStringLiteral("roleName")).toString(peer.displayName);
        const QString text = payload.value(QStringLiteral("text")).toString();
        emit chatMessageReceived(peer, roleName, text);
    } else if (type == QStringLiteral("file")) {
        handleFileMessage(peer, payload);
    } else if (type == QStringLiteral("share_request")) {
        handleShareRequest(peer, payload);
    } else if (type == QStringLiteral("share_catalog")) {
        handleShareCatalog(peer, payload);
    } else if (type == QStringLiteral("share_download")) {
        handleShareDownload(peer, payload);
    }
}

void ChatController::handleFileMessage(const PeerInfo &peer, const QJsonObject &payload) {
    const QString fileName = payload.value(QStringLiteral("fileName")).toString();
    const QByteArray data = QByteArray::fromBase64(payload.value(QStringLiteral("data")).toString().toLatin1());
    if (fileName.isEmpty() || data.isEmpty()) {
        return;
    }
    const QString localPath = saveIncomingFile(fileName, data);
    if (!localPath.isEmpty()) {
        const QString roleName = payload.value(QStringLiteral("roleName")).toString(peer.displayName);
        emit fileReceived(peer, roleName, fileName, localPath);
        emit statusInfo(tr("已保存来自 %1 的文件 %2").arg(peer.displayName, fileName));
    }
}

void ChatController::handleShareCatalog(const PeerInfo &peer, const QJsonObject &payload) {
    QList<SharedFileInfo> files;
    const QJsonArray array = payload.value(QStringLiteral("files")).toArray();
    for (const QJsonValue &value : array) {
        const QJsonObject entry = value.toObject();
        SharedFileInfo info;
        info.entryId = entry.value(QStringLiteral("entryId")).toString();
        info.name = entry.value(QStringLiteral("name")).toString();
        info.size = static_cast<quint64>(entry.value(QStringLiteral("size")).toDouble());
        files.append(info);
    }
    m_remoteShareCatalogs.insert(peer.id, files);
    emit shareCatalogReceived(peer.id, files);
}

void ChatController::handleShareRequest(const PeerInfo &peer, const QJsonObject &) {
    sendShareCatalogToPeer(peer);
}

void ChatController::handleShareDownload(const PeerInfo &peer, const QJsonObject &payload) {
    const QString entryId = payload.value(QStringLiteral("entryId")).toString();
    if (entryId.isEmpty()) {
        return;
    }
    if (!m_localShareIndex.contains(entryId)) {
        emit controllerWarning(tr("共享条目不存在或已失效"));
        return;
    }
    const SharedFileInfo info = m_localShareIndex.value(entryId);
    sendFileToPeer(peer.id, info.filePath);
}

void ChatController::sendShareCatalogToPeer(const PeerInfo &peer) {
    const QList<SharedFileInfo> files = collectLocalShares();
    QJsonArray array;
    for (const SharedFileInfo &info : files) {
        array.append(QJsonObject{
            {QStringLiteral("entryId"), info.entryId},
            {QStringLiteral("name"), info.name},
            {QStringLiteral("size"), static_cast<double>(info.size)}
        });
    }
    QJsonObject payload{
        {QStringLiteral("type"), QStringLiteral("share_catalog")},
        {QStringLiteral("files"), array}
    };
    m_router.sendSharePayload(peer, payload);
}

QList<SharedFileInfo> ChatController::collectLocalShares() {
    QList<SharedFileInfo> files;
    m_localShareIndex.clear();
    for (const QString &path : m_settings.sharedDirectories) {
        QDir dir(path);
        if (!dir.exists()) {
            continue;
        }
        const QFileInfoList fileInfos = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        for (const QFileInfo &info : fileInfos) {
            SharedFileInfo entry;
            entry.name = info.fileName();
            entry.size = static_cast<quint64>(info.size());
            entry.filePath = info.absoluteFilePath();
            entry.entryId = buildShareEntryId(entry.filePath);
            files.append(entry);
            m_localShareIndex.insert(entry.entryId, entry);
        }
    }
    return files;
}

QString ChatController::saveIncomingFile(const QString &fileName, const QByteArray &data) {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("downloads"));
    }
    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QString resolvedName = fileName;
    QString targetPath = dir.filePath(resolvedName);
    int suffix = 1;
    while (QFile::exists(targetPath)) {
        const QString base = QFileInfo(fileName).completeBaseName();
        const QString ext = QFileInfo(fileName).suffix();
        resolvedName = QStringLiteral("%1_%2.%3").arg(base).arg(suffix++).arg(ext);
        targetPath = dir.filePath(resolvedName);
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit controllerWarning(tr("无法保存文件: %1").arg(file.errorString()));
        return {};
    }
    file.write(data);
    file.close();
    return targetPath;
}

QString ChatController::buildShareEntryId(const QString &filePath) const {
    const QByteArray hash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Sha1);
    return QString::fromLatin1(hash.toHex());
}
