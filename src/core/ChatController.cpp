#include "ChatController.h"

#include "LanguageKeys.h"
#include "LanguageManager.h"

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
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

int maxPrefixFor(const QHostAddress &address) {
    return address.protocol() == QAbstractSocket::IPv6Protocol ? 128 : 32;
}

QHostAddress normalizeNetwork(const QHostAddress &address, int prefixLength) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol && prefixLength >= 0 && prefixLength <= 32) {
        const quint32 ip = address.toIPv4Address();
        const quint32 mask = prefixLength == 0 ? 0 : 0xFFFFFFFFu << (32 - prefixLength);
        return QHostAddress(ip & mask);
    }
    return address;
}

QString subnetToken(const QHostAddress &network, int prefixLength) {
    return QStringLiteral("%1/%2").arg(network.toString()).arg(prefixLength);
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
    settings.searchPort =
        static_cast<quint16>(jsonInt(object, QStringLiteral("searchPort"), settings.searchPort));
    settings.organizationCode = object.value(QStringLiteral("organizationCode")).toString(settings.organizationCode);
    settings.enableInterop = jsonBool(object, QStringLiteral("enableInterop"), settings.enableInterop);
    settings.interopPort =
        static_cast<quint16>(jsonInt(object, QStringLiteral("interopPort"), settings.interopPort));
    settings.bindNetworkInterface =
        jsonBool(object, QStringLiteral("bindInterface"), settings.bindNetworkInterface);
    settings.boundInterfaceId = object.value(QStringLiteral("interfaceId")).toString(settings.boundInterfaceId);
    settings.restrictToListedSubnets =
        jsonBool(object, QStringLiteral("restrictToListedSubnets"), settings.restrictToListedSubnets);
    settings.autoRefresh = jsonBool(object, QStringLiteral("autoRefresh"), settings.autoRefresh);
    settings.refreshIntervalMinutes = jsonInt(object, QStringLiteral("refreshInterval"), settings.refreshIntervalMinutes);
    return settings;
}

QJsonObject networkSettingsToJson(const NetworkSettings &settings) {
    return {
        {QStringLiteral("searchPort"), static_cast<int>(settings.searchPort)},
        {QStringLiteral("organizationCode"), settings.organizationCode},
        {QStringLiteral("enableInterop"), settings.enableInterop},
        {QStringLiteral("interopPort"), static_cast<int>(settings.interopPort)},
        {QStringLiteral("bindInterface"), settings.bindNetworkInterface},
        {QStringLiteral("interfaceId"), settings.boundInterfaceId},
        {QStringLiteral("restrictToListedSubnets"), settings.restrictToListedSubnets},
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

ChatController::~ChatController() {
    m_discovery.stop();
    m_router.stop();
}

bool ChatController::initialize() {
    m_storageReady = m_storage.initialize(databaseFilePath());
    if (!m_storageReady) {
        emit controllerWarning(LanguageManager::text(
            LangKey::Controller::CannotWriteConfig, QStringLiteral("无法初始化配置数据库，设置将不会持久化。")));
    }
    loadSettings();
    initializeRoles();
    if (m_settings.activeRoleId.isEmpty() && !m_roles.isEmpty()) {
        m_settings.activeRoleId = m_roles.front().id;
    }

    if (!m_router.startListening(m_listenPort)) {
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::RouterFailed, QStringLiteral("消息路由启动失败")));
    }
    m_router.setLocalPeerId(m_localId);
    m_router.setLocalDisplayName(m_displayName);

    m_discovery.setLocalIdentity(m_localId, m_displayName, m_listenPort);
    m_discovery.setSubnets(m_subnets);
    m_discovery.setBlockedSubnets(m_blockedSubnets);
    m_discovery.start();
    m_discovery.announceOnline();

    const QString readyText =
        LanguageManager::text(LangKey::Controller::StartupReady, QStringLiteral("启动完成，ID: %1 端口: %2"))
            .arg(m_localId)
            .arg(m_listenPort);
    emit statusInfo(readyText);
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

QList<QPair<QHostAddress, int>> ChatController::blockedSubnets() const {
    return m_blockedSubnets;
}

const AppSettings &ChatController::settings() const {
    return m_settings;
}

ProfileDetails ChatController::profileDetails() const {
    return m_settings.profile;
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
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::PeerMissing, QStringLiteral("未找到联系人 %1")).arg(peerId));
        return;
    }
    const ProfileDetails profile = m_settings.profile;
    const QString roleName = profile.name.isEmpty() ? m_displayName : profile.name;
    m_router.sendChatMessage(peer, text, QString(), roleName);
    recordChatHistory(peer.id, roleName, text, MessageDirection::Outgoing, QStringLiteral("chat"));
}

void ChatController::sendFileToPeer(const QString &peerId, const QString &filePath) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::PeerMissing, QStringLiteral("未找到联系人 %1")).arg(peerId));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::CannotReadFile, QStringLiteral("无法读取文件: %1"))
                .arg(file.errorString()));
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
    emit statusInfo(
        LanguageManager::text(LangKey::Controller::FileSent, QStringLiteral("已发送文件 %1")).arg(file.fileName()));
    recordChatHistory(peer.id, roleName, QFileInfo(file).fileName(), MessageDirection::Outgoing,
                      QStringLiteral("file"), filePath);
}

void ChatController::requestPeerShareList(const QString &peerId) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::PeerMissing, QStringLiteral("未找到联系人 %1")).arg(peerId));
        return;
    }
    QJsonObject payload{{QStringLiteral("type"), QStringLiteral("share_request")}};
    m_router.sendSharePayload(peer, payload);
}

void ChatController::requestPeerSharedFile(const QString &peerId, const QString &entryId) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::PeerMissing, QStringLiteral("未找到联系人 %1")).arg(peerId));
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
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::PeerMissing, QStringLiteral("未找到联系人 %1")).arg(peerId));
        return;
    }
    sendShareCatalogToPeer(peer);
}

void ChatController::addSubnet(const QHostAddress &network, int prefixLength) {
    QList<QPair<QHostAddress, int>> next = m_subnets;
    const QString token = QStringLiteral("%1/%2").arg(network.toString()).arg(prefixLength);
    for (const auto &pair : std::as_const(next)) {
        const QString existing = QStringLiteral("%1/%2").arg(pair.first.toString()).arg(pair.second);
        if (existing == token) {
            return;
        }
    }
    next.append(qMakePair(network, prefixLength));
    setSubnets(next);
}

void ChatController::setSubnets(const QList<QPair<QHostAddress, int>> &subnets) {
    QList<QPair<QHostAddress, int>> sanitized;
    QSet<QString> seen;
    for (const auto &pair : subnets) {
        if (pair.first.isNull()) {
            continue;
        }
        const int maxPrefix = maxPrefixFor(pair.first);
        if (pair.second < 0 || pair.second > maxPrefix) {
            continue;
        }
        QHostAddress normalized = normalizeNetwork(pair.first, pair.second);
        const QString token = subnetToken(normalized, pair.second);
        if (seen.contains(token)) {
            continue;
        }
        seen.insert(token);
        sanitized.append(qMakePair(normalized, pair.second));
    }
    m_subnets = sanitized;
    m_discovery.setSubnets(m_subnets);
    m_discovery.setBlockedSubnets(m_blockedSubnets);
    for (const auto &range : std::as_const(m_subnets)) {
        m_discovery.probeSubnet(range.first, range.second);
    }
    m_discovery.announceOnline();
    persistSettings();
}

void ChatController::setBlockedSubnets(const QList<QPair<QHostAddress, int>> &subnets) {
    QList<QPair<QHostAddress, int>> sanitized;
    QSet<QString> seen;
    for (const auto &pair : subnets) {
        if (pair.first.isNull()) {
            continue;
        }
        const int maxPrefix = maxPrefixFor(pair.first);
        if (pair.second < 0 || pair.second > maxPrefix) {
            continue;
        }
        QHostAddress normalized = normalizeNetwork(pair.first, pair.second);
        const QString token = subnetToken(normalized, pair.second);
        if (seen.contains(token)) {
            continue;
        }
        seen.insert(token);
        sanitized.append(qMakePair(normalized, pair.second));
    }
    m_blockedSubnets = sanitized;
    m_discovery.setBlockedSubnets(m_blockedSubnets);
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
    m_shareManager.collectLocalShares(m_settings.sharedDirectories);
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateSignatureText(const QString &signature) {
    m_settings.signatureText = signature;
    persistSettings();
    emit preferencesChanged(m_settings);
}

void ChatController::updateProfileDetails(const ProfileDetails &details) {
    ProfileDetails updated = details;
    if (updated.unit.isEmpty()) {
        updated.unit = QStringLiteral("生物研究院");
    }
    if (updated.department.isEmpty()) {
        updated.department = QStringLiteral("神经网络研究室");
    }
    m_settings.profile = updated;
    if (!updated.name.isEmpty()) {
        m_displayName = updated.name;
    }
    m_settings.signatureText = updated.signature;
    persistSettings();
    emit profileUpdated(updated);
    emit preferencesChanged(m_settings);
}

QString ChatController::databaseFilePath() const {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QCoreApplication::applicationDirPath();
    }
    QDir dir(baseDir);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("lan_chat_data.sqlite"));
}

QString ChatController::legacyConfigFilePath() const {
    const QString baseDir = QCoreApplication::applicationDirPath();
    return QDir(baseDir).filePath(QStringLiteral("lan_chat_settings.json"));
}

PersistedState ChatController::loadLegacyState() const {
    PersistedState state;
    QFile file(legacyConfigFilePath());
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const QJsonObject root = doc.object();
        const QJsonObject identity = root.value(QStringLiteral("identity")).toObject();
        const QJsonObject network = root.value(QStringLiteral("network")).toObject();
        const QJsonObject preferences = root.value(QStringLiteral("preferences")).toObject();

        state.localId = identity.value(QStringLiteral("id")).toString();
        state.displayName = identity.value(QStringLiteral("name")).toString();
        const int portValue = network.value(QStringLiteral("listenPort")).toInt(state.listenPort);
        if (portValue > 0 && portValue <= std::numeric_limits<quint16>::max()) {
            state.listenPort = static_cast<quint16>(portValue);
        }

        const QJsonArray subnetArray = network.value(QStringLiteral("subnets")).toArray();
        for (const QJsonValue &value : subnetArray) {
            const QJsonObject entry = value.toObject();
            QHostAddress addr(entry.value(QStringLiteral("network")).toString());
            const int prefix = entry.value(QStringLiteral("prefix")).toInt(-1);
            if (!addr.isNull() && prefix >= 0 && prefix <= maxPrefixFor(addr)) {
                state.subnets.append(qMakePair(normalizeNetwork(addr, prefix), prefix));
            }
        }
        const QJsonArray blockedArray = network.value(QStringLiteral("blockedSubnets")).toArray();
        for (const QJsonValue &value : blockedArray) {
            const QJsonObject entry = value.toObject();
            QHostAddress addr(entry.value(QStringLiteral("network")).toString());
            const int prefix = entry.value(QStringLiteral("prefix")).toInt(-1);
            if (!addr.isNull() && prefix >= 0 && prefix <= maxPrefixFor(addr)) {
                state.blockedSubnets.append(qMakePair(normalizeNetwork(addr, prefix), prefix));
            }
        }

        state.settings.general = parseGeneral(preferences.value(QStringLiteral("general")).toObject());
        state.settings.network = parseNetworkSettings(preferences.value(QStringLiteral("networkPrefs")).toObject());
        state.settings.notifications =
            parseNotificationSettings(preferences.value(QStringLiteral("notifications")).toObject());
        state.settings.hotkeys = parseHotkeySettings(preferences.value(QStringLiteral("hotkeys")).toObject());
        state.settings.security = parseSecuritySettings(preferences.value(QStringLiteral("security")).toObject());
        state.settings.mail = parseMailSettings(preferences.value(QStringLiteral("mail")).toObject());
        state.settings.sharedDirectories =
            parseStringList(preferences.value(QStringLiteral("sharedFolders")).toArray());
        state.settings.activeRoleId = preferences.value(QStringLiteral("activeRoleId")).toString();
        state.settings.signatureText =
            preferences.value(QStringLiteral("signature")).toString(QStringLiteral("编辑个性签名"));
        const QString profileNameFallback =
            state.displayName.isEmpty() ? state.localId : state.displayName;
        state.settings.profile = parseProfileObject(preferences.value(QStringLiteral("profile")).toObject(),
                                                    profileNameFallback, state.settings.signatureText);
    }
    return state;
}

void ChatController::loadSettings() {
    PersistedState state;
    bool hasStoredState = false;
    bool migratedFromLegacy = false;
    if (m_storageReady) {
        state = m_storage.loadState();
        hasStoredState = !state.localId.isEmpty();
    }
    if (!hasStoredState) {
        state = loadLegacyState();
        migratedFromLegacy = !state.localId.isEmpty();
    }

    m_localId = state.localId;
    m_displayName = state.displayName;
    m_listenPort = state.listenPort;
    m_subnets = state.subnets;
    m_blockedSubnets = state.blockedSubnets;
    m_settings = state.settings;
    m_hasStoredRole = !m_settings.activeRoleId.isEmpty();

    if (m_localId.isEmpty()) {
        m_localId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (m_displayName.isEmpty()) {
        m_displayName = QStringLiteral("EVA-0");
    }
    if (m_settings.signatureText.isEmpty()) {
        m_settings.signatureText = QStringLiteral("编辑个性签名");
    }
    if (m_settings.profile.name.isEmpty()) {
        m_settings.profile.name = m_displayName;
    }
    if (m_settings.profile.signature.isEmpty()) {
        m_settings.profile.signature = m_settings.signatureText;
    }
    if (m_settings.profile.version.isEmpty()) {
        m_settings.profile.version = QCoreApplication::applicationVersion();
        if (m_settings.profile.version.isEmpty()) {
            m_settings.profile.version = QStringLiteral("1.0.0");
        }
    }
    if (m_settings.profile.ip.isEmpty()) {
        m_settings.profile.ip = QStringLiteral("192.168.0.2");
    }
    if (m_settings.profile.unit.isEmpty()) {
        m_settings.profile.unit = QStringLiteral("生物研究院");
    }
    if (m_settings.profile.department.isEmpty()) {
        m_settings.profile.department = QStringLiteral("神经网络研究室");
    }
    m_displayName = m_settings.profile.name;
    m_shareManager.collectLocalShares(m_settings.sharedDirectories);
    if (migratedFromLegacy && m_storageReady) {
        persistSettings();
    }
}

void ChatController::recordChatHistory(const QString &peerId, const QString &roleName, const QString &content,
                                       MessageDirection direction, const QString &messageType,
                                       const QString &attachmentPath) {
    if (!m_storageReady) {
        return;
    }
    StoredMessage message;
    message.peerId = peerId;
    message.roleName = roleName;
    message.content = content;
    message.attachmentPath = attachmentPath;
    message.direction = direction;
    message.messageType = messageType;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    m_storage.storeMessage(message);
}

void ChatController::persistSettings() {
    if (m_storageReady) {
        PersistedState state;
        state.localId = m_localId;
        state.displayName = m_displayName;
        state.listenPort = m_listenPort;
        state.subnets = m_subnets;
        state.blockedSubnets = m_blockedSubnets;
        state.settings = m_settings;
        m_storage.saveState(state);
        return;
    }

    QFile file(legacyConfigFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::CannotWriteConfig, QStringLiteral("无法写入配置: %1"))
                .arg(file.errorString()));
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
    QJsonArray blockedArray;
    for (const auto &pair : std::as_const(m_blockedSubnets)) {
        blockedArray.append(QJsonObject{
            {QStringLiteral("network"), pair.first.toString()},
            {QStringLiteral("prefix"), pair.second}
        });
    }

    QJsonObject network{
        {QStringLiteral("listenPort"), static_cast<int>(m_listenPort)},
        {QStringLiteral("subnets"), subnetArray},
        {QStringLiteral("blockedSubnets"), blockedArray}
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
        {QStringLiteral("signature"), m_settings.signatureText},
        {QStringLiteral("profile"), profileToJson(m_settings.profile)}
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
        recordChatHistory(peer.id, roleName, text, MessageDirection::Incoming, QStringLiteral("chat"));
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
    QString errorString;
    const QString localPath = m_shareManager.saveIncomingFile(fileName, data, &errorString);
    if (localPath.isEmpty()) {
        if (!errorString.isEmpty()) {
            emit controllerWarning(
                LanguageManager::text(LangKey::Controller::CannotSaveFile, QStringLiteral("无法保存文件: %1"))
                    .arg(errorString));
        }
        return;
    }
    const QString roleName = payload.value(QStringLiteral("roleName")).toString(peer.displayName);
    emit fileReceived(peer, roleName, fileName, localPath);
    emit statusInfo(LanguageManager::text(LangKey::Controller::FileSaved, QStringLiteral("已保存来自 %1 的文件 %2"))
                        .arg(peer.displayName, fileName));
    recordChatHistory(peer.id, roleName, fileName, MessageDirection::Incoming, QStringLiteral("file"), localPath);
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
    m_shareManager.updateRemoteCatalog(peer.id, files);
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
    if (!m_shareManager.hasLocalEntry(entryId)) {
        emit controllerWarning(
            LanguageManager::text(LangKey::Controller::ShareMissing, QStringLiteral("共享条目不存在或已失效")));
        return;
    }
    const SharedFileInfo info = m_shareManager.localEntry(entryId);
    sendFileToPeer(peer.id, info.filePath);
}

void ChatController::sendShareCatalogToPeer(const PeerInfo &peer) {
    const QList<SharedFileInfo> files = m_shareManager.collectLocalShares(m_settings.sharedDirectories);
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

ProfileDetails ChatController::parseProfileObject(const QJsonObject &object, const QString &nameFallback,
                                                  const QString &signatureFallback) const {
    ProfileDetails details;
    details.name = object.value(QStringLiteral("name")).toString(nameFallback);
    details.signature = object.value(QStringLiteral("signature")).toString(signatureFallback);
    details.gender = object.value(QStringLiteral("gender")).toString(QStringLiteral("男"));
    details.unit = object.value(QStringLiteral("unit")).toString(QStringLiteral("生物研究院"));
    details.department = object.value(QStringLiteral("department")).toString(QStringLiteral("神经网络研究室"));
    details.phone = object.value(QStringLiteral("phone")).toString();
    details.mobile = object.value(QStringLiteral("mobile")).toString();
    details.email = object.value(QStringLiteral("email")).toString();
    details.version = object.value(QStringLiteral("version")).toString();
    details.ip = object.value(QStringLiteral("ip")).toString();
    return details;
}

QJsonObject ChatController::profileToJson(const ProfileDetails &details) const {
    return QJsonObject{
        {QStringLiteral("name"), details.name},
        {QStringLiteral("signature"), details.signature},
        {QStringLiteral("gender"), details.gender},
        {QStringLiteral("unit"), details.unit},
        {QStringLiteral("department"), details.department},
        {QStringLiteral("phone"), details.phone},
        {QStringLiteral("mobile"), details.mobile},
        {QStringLiteral("email"), details.email},
        {QStringLiteral("version"), details.version},
        {QStringLiteral("ip"), details.ip}
    };
}
