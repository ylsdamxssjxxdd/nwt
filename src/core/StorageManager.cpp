#include "StorageManager.h"

#include "PeerInfo.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <limits>

namespace {
int boolToInt(bool value) {
    return value ? 1 : 0;
}

bool intToBool(int value) {
    return value != 0;
}

QList<QPair<QHostAddress, int>> readSubnets(QSqlDatabase &db, bool blocked) {
    QList<QPair<QHostAddress, int>> list;
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT network, prefix FROM network_subnets WHERE type = ? ORDER BY id ASC"));
    query.addBindValue(blocked ? 1 : 0);
    if (query.exec()) {
        while (query.next()) {
            QHostAddress addr(query.value(0).toString());
            const int prefix = query.value(1).toInt();
            if (!addr.isNull() && prefix >= 0) {
                list.append(qMakePair(addr, prefix));
            }
        }
    }
    return list;
}
} // namespace

StorageManager::StorageManager() {
    m_connectionName = QStringLiteral("nwt_storage_%1")
                           .arg(reinterpret_cast<quintptr>(this), 0, 16);
}

StorageManager::~StorageManager() {
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid()) {
            db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool StorageManager::initialize(const QString &databasePath) {
    m_databasePath = databasePath;
    QFileInfo info(databasePath);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return false;
    }

    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(databasePath);
    if (!db.open()) {
        return false;
    }
    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
    ensureSchema(db);
    m_initialized = true;
    return true;
}

bool StorageManager::isReady() const {
    return m_initialized;
}

QString StorageManager::databasePath() const {
    return m_databasePath;
}

PersistedState StorageManager::loadState() const {
    PersistedState state;
    if (!m_initialized) {
        return state;
    }
    QSqlDatabase db = connection();
    if (!db.isValid()) {
        return state;
    }

    QSqlQuery query(db);
    if (query.exec(QStringLiteral("SELECT id, display_name, listen_port FROM app_identity LIMIT 1"))) {
        if (query.next()) {
            state.localId = query.value(0).toString();
            state.displayName = query.value(1).toString();
            const int port = query.value(2).toInt();
            if (port > 0 && port <= std::numeric_limits<quint16>::max()) {
                state.listenPort = static_cast<quint16>(port);
            }
        }
    }

    state.subnets = readSubnets(db, false);
    state.blockedSubnets = readSubnets(db, true);

    if (query.exec(QStringLiteral("SELECT * FROM general_settings WHERE id = 1"))) {
        if (query.next()) {
            GeneralSettings &general = state.settings.general;
            general.autoStart = intToBool(query.value(QStringLiteral("auto_start")).toInt());
            general.enableLanUpdate = intToBool(query.value(QStringLiteral("lan_update")).toInt());
            general.rejectShake = intToBool(query.value(QStringLiteral("reject_shake")).toInt());
            general.rejectFeigeAds = intToBool(query.value(QStringLiteral("reject_feige_ads")).toInt());
            general.enableFeigeGroup = intToBool(query.value(QStringLiteral("enable_feige_group")).toInt());
            general.autoMinimize = intToBool(query.value(QStringLiteral("auto_minimize")).toInt());
            general.hideIpAddress = intToBool(query.value(QStringLiteral("hide_ip_address")).toInt());
            general.rejectSegmentShare = intToBool(query.value(QStringLiteral("reject_segment_share")).toInt());
            general.enableAutoGroup = intToBool(query.value(QStringLiteral("auto_group")).toInt());
            general.enableAutoSubGroup = intToBool(query.value(QStringLiteral("auto_sub_group")).toInt());
            general.popupOnMessage = intToBool(query.value(QStringLiteral("popup_on_message")).toInt());
            general.flashOnMessage = intToBool(query.value(QStringLiteral("flash_on_message")).toInt());
            const QString closeBehavior = query.value(QStringLiteral("close_behavior")).toString();
            if (!closeBehavior.isEmpty()) {
                general.closeBehavior = closeBehavior;
            }
            const QString friendDisplay = query.value(QStringLiteral("friend_display")).toString();
            if (!friendDisplay.isEmpty()) {
                general.friendDisplay = friendDisplay;
            }
        }
    }

    if (query.exec(QStringLiteral("SELECT * FROM network_settings WHERE id = 1"))) {
        if (query.next()) {
            NetworkSettings &network = state.settings.network;
            const int searchPort = query.value(QStringLiteral("search_port")).toInt();
            if (searchPort > 0 && searchPort <= std::numeric_limits<quint16>::max()) {
                network.searchPort = static_cast<quint16>(searchPort);
            }
            network.organizationCode = query.value(QStringLiteral("organization_code")).toString();
            network.enableInterop = intToBool(query.value(QStringLiteral("enable_interop")).toInt());
            const int interopPort = query.value(QStringLiteral("interop_port")).toInt();
            if (interopPort > 0 && interopPort <= std::numeric_limits<quint16>::max()) {
                network.interopPort = static_cast<quint16>(interopPort);
            }
            network.bindNetworkInterface = intToBool(query.value(QStringLiteral("bind_interface")).toInt());
            network.boundInterfaceId = query.value(QStringLiteral("interface_id")).toString();
            network.restrictToListedSubnets = intToBool(query.value(QStringLiteral("restrict_listed")).toInt());
            network.autoRefresh = intToBool(query.value(QStringLiteral("auto_refresh")).toInt());
            const int refreshInterval = query.value(QStringLiteral("refresh_interval")).toInt();
            if (refreshInterval > 0) {
                network.refreshIntervalMinutes = refreshInterval;
            }
        }
    }

    if (query.exec(QStringLiteral("SELECT * FROM notification_settings WHERE id = 1"))) {
        if (query.next()) {
            NotificationSettings &notifications = state.settings.notifications;
            notifications.notifySelfOnline = intToBool(query.value(QStringLiteral("notify_self_online")).toInt());
            notifications.notifyPeerOnline = intToBool(query.value(QStringLiteral("notify_peer_online")).toInt());
            notifications.notifyIncomingMessage = intToBool(query.value(QStringLiteral("notify_incoming")).toInt());
            notifications.notifyFileTransfer = intToBool(query.value(QStringLiteral("notify_file_transfer")).toInt());
            notifications.notifyMailChange = intToBool(query.value(QStringLiteral("notify_mail")).toInt());
            notifications.muteAll = intToBool(query.value(QStringLiteral("mute_all")).toInt());
            notifications.playGroupSound = intToBool(query.value(QStringLiteral("play_group_sound")).toInt());
        }
    }

    if (query.exec(QStringLiteral("SELECT * FROM hotkey_settings WHERE id = 1"))) {
        if (query.next()) {
            HotkeySettings &hotkeys = state.settings.hotkeys;
            hotkeys.toggleMainWindow = intToBool(query.value(QStringLiteral("toggle_main_window")).toInt());
            const QString toggleShortcut = query.value(QStringLiteral("toggle_shortcut")).toString();
            if (!toggleShortcut.isEmpty()) {
                hotkeys.toggleShortcut = toggleShortcut;
            }
            hotkeys.captureEnabled = intToBool(query.value(QStringLiteral("capture_enabled")).toInt());
            const QString captureShortcut = query.value(QStringLiteral("capture_shortcut")).toString();
            if (!captureShortcut.isEmpty()) {
                hotkeys.captureShortcut = captureShortcut;
            }
            hotkeys.lockEnabled = intToBool(query.value(QStringLiteral("lock_enabled")).toInt());
            const QString lockShortcut = query.value(QStringLiteral("lock_shortcut")).toString();
            if (!lockShortcut.isEmpty()) {
                hotkeys.lockShortcut = lockShortcut;
            }
            hotkeys.openStorageEnabled = intToBool(query.value(QStringLiteral("open_storage_enabled")).toInt());
            const QString openStorageShortcut = query.value(QStringLiteral("open_storage_shortcut")).toString();
            if (!openStorageShortcut.isEmpty()) {
                hotkeys.openStorageShortcut = openStorageShortcut;
            }
        }
    }

    if (query.exec(QStringLiteral("SELECT * FROM security_settings WHERE id = 1"))) {
        if (query.next()) {
            SecuritySettings &security = state.settings.security;
            security.lockEnabled = intToBool(query.value(QStringLiteral("lock_enabled")).toInt());
            security.passwordHash = query.value(QStringLiteral("password_hash")).toString();
            const int idleMinutes = query.value(QStringLiteral("idle_minutes")).toInt();
            if (idleMinutes > 0) {
                security.idleMinutes = idleMinutes;
            }
            security.idleAction =
                query.value(QStringLiteral("idle_action")).toInt() == 1 ? SecuritySettings::LockApplication
                                                                       : SecuritySettings::SwitchAway;
        }
    }

    if (query.exec(QStringLiteral("SELECT * FROM mail_settings WHERE id = 1"))) {
        if (query.next()) {
            MailSettings &mail = state.settings.mail;
            mail.address = query.value(QStringLiteral("address")).toString();
            mail.imapServer = query.value(QStringLiteral("imap_server")).toString();
            mail.smtpServer = query.value(QStringLiteral("smtp_server")).toString();
            const int mailPort = query.value(QStringLiteral("port")).toInt();
            if (mailPort > 0) {
                mail.port = mailPort;
            }
            mail.playSoundOnMail = intToBool(query.value(QStringLiteral("play_sound")).toInt());
            mail.popupOnMail = intToBool(query.value(QStringLiteral("popup_on_mail")).toInt());
        }
    }

    state.settings.sharedDirectories.clear();
    if (query.exec(QStringLiteral("SELECT path FROM shared_directories ORDER BY id ASC"))) {
        while (query.next()) {
            state.settings.sharedDirectories.append(query.value(0).toString());
        }
    }

    if (query.exec(QStringLiteral("SELECT * FROM profile_details WHERE id = 1"))) {
        if (query.next()) {
            ProfileDetails &profile = state.settings.profile;
            profile.name = query.value(QStringLiteral("name")).toString();
            profile.signature = query.value(QStringLiteral("signature")).toString();
            const QString gender = query.value(QStringLiteral("gender")).toString();
            if (!gender.isEmpty()) {
                profile.gender = gender;
            }
            profile.avatarPath = query.value(QStringLiteral("avatar_path")).toString();
            const QString unit = query.value(QStringLiteral("unit")).toString();
            if (!unit.isEmpty()) {
                profile.unit = unit;
            }
            const QString department = query.value(QStringLiteral("department")).toString();
            if (!department.isEmpty()) {
                profile.department = department;
            }
            profile.phone = query.value(QStringLiteral("phone")).toString();
            profile.mobile = query.value(QStringLiteral("mobile")).toString();
            profile.email = query.value(QStringLiteral("email")).toString();
            profile.version = query.value(QStringLiteral("version")).toString();
            profile.ip = query.value(QStringLiteral("ip")).toString();
        }
    }

    if (query.exec(QStringLiteral("SELECT active_role_id, signature_text FROM app_state WHERE id = 1"))) {
        if (query.next()) {
            state.settings.activeRoleId = query.value(0).toString();
            const QString signatureText = query.value(1).toString();
            if (!signatureText.isEmpty()) {
                state.settings.signatureText = signatureText;
            }
        }
    }
    return state;
}

void StorageManager::saveState(const PersistedState &state) {
    if (!m_initialized) {
        return;
    }
    QSqlDatabase db = connection();
    if (!db.isValid()) {
        return;
    }

    db.transaction();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("REPLACE INTO app_identity(id, display_name, listen_port, updated_at) VALUES (?, ?, ?, ?)"));
    query.addBindValue(state.localId);
    query.addBindValue(state.displayName);
    query.addBindValue(static_cast<int>(state.listenPort));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.exec();

    query.exec(QStringLiteral("DELETE FROM network_subnets"));
    writeSubnets(state.subnets, false, db);
    writeSubnets(state.blockedSubnets, true, db);

    writeGeneralSettings(state.settings, db);
    writeNetworkSettings(state.settings, db);
    writeNotificationSettings(state.settings, db);
    writeHotkeySettings(state.settings, db);
    writeSecuritySettings(state.settings, db);
    writeMailSettings(state.settings, db);
    writeSharedDirectories(state.settings.sharedDirectories, db);
    writeProfile(state.settings, db);

    query.prepare(QStringLiteral("REPLACE INTO app_state(id, active_role_id, signature_text) VALUES (1, ?, ?)"));
    query.addBindValue(state.settings.activeRoleId);
    query.addBindValue(state.settings.signatureText);
    query.exec();

    if (!db.commit()) {
        db.rollback();
    }
}

void StorageManager::storeMessage(const StoredMessage &message) {
    if (!m_initialized || message.peerId.isEmpty()) {
        return;
    }
    QSqlDatabase db = connection();
    if (!db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO chat_messages(peer_id, role_name, message_type, content, attachment_path, "
                                 "outgoing, created_at) VALUES (?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(message.peerId);
    query.addBindValue(message.roleName);
    query.addBindValue(message.messageType);
    query.addBindValue(message.content);
    query.addBindValue(message.attachmentPath);
    query.addBindValue(message.direction == MessageDirection::Outgoing ? 1 : 0);
    const qint64 timestamp =
        message.timestamp == 0 ? QDateTime::currentSecsSinceEpoch() : message.timestamp;
    query.addBindValue(timestamp);
    query.exec();
}

QVector<StoredMessage> StorageManager::recentMessages(const QString &peerId, int limit) const {
    QVector<StoredMessage> messages;
    if (!m_initialized || peerId.isEmpty()) {
        return messages;
    }
    QSqlDatabase db = connection();
    if (!db.isValid()) {
        return messages;
    }
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id, role_name, message_type, content, attachment_path, outgoing, created_at "
                                 "FROM chat_messages WHERE peer_id = ? ORDER BY created_at DESC LIMIT ?"));
    query.addBindValue(peerId);
    query.addBindValue(limit);
    if (query.exec()) {
        while (query.next()) {
            StoredMessage msg;
            msg.id = query.value(0).toLongLong();
            msg.peerId = peerId;
            msg.roleName = query.value(1).toString();
            msg.messageType = query.value(2).toString();
            msg.content = query.value(3).toString();
            msg.attachmentPath = query.value(4).toString();
            msg.direction = query.value(5).toInt() == 1 ? MessageDirection::Outgoing : MessageDirection::Incoming;
            msg.timestamp = query.value(6).toLongLong();
            messages.append(msg);
        }
    }
    return messages;
}

QVector<QString> StorageManager::recentPeerIds(int limit) const {
    QVector<QString> peers;
    if (!m_initialized || limit <= 0) {
        return peers;
    }
    QSqlDatabase db = connection();
    if (!db.isValid()) {
        return peers;
    }
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT peer_id, MAX(created_at) AS last_ts "
                                 "FROM chat_messages "
                                 "GROUP BY peer_id "
                                 "ORDER BY last_ts DESC "
                                 "LIMIT ?"));
    query.addBindValue(limit);
    if (query.exec()) {
        while (query.next()) {
            const QString peerId = query.value(0).toString();
            if (!peerId.isEmpty()) {
                peers.append(peerId);
            }
        }
    }
    return peers;
}

void StorageManager::upsertKnownPeer(const PeerInfo &peer) {
    if (!m_initialized || peer.id.isEmpty()) {
        return;
    }
    QSqlDatabase db = connection();
    if (!db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    query.prepare(QStringLiteral("REPLACE INTO known_peers(peer_id, display_name, address, listen_port, last_seen,"
                                 " capabilities) VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(peer.id);
    query.addBindValue(peer.displayName);
    query.addBindValue(peer.address.toString());
    query.addBindValue(static_cast<int>(peer.listenPort));
    const qint64 ts = peer.lastSeen.isValid() ? peer.lastSeen.toSecsSinceEpoch()
                                              : QDateTime::currentSecsSinceEpoch();
    query.addBindValue(ts);
    query.addBindValue(peer.capabilities);
    query.exec();
}

QList<PeerInfo> StorageManager::knownPeers() const {
    QList<PeerInfo> list;
    if (!m_initialized) {
        return list;
    }
    QSqlDatabase db = connection();
    if (!db.isValid()) {
        return list;
    }
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT peer_id, display_name, address, listen_port, last_seen, capabilities"
                                   " FROM known_peers ORDER BY last_seen DESC"))) {
        return list;
    }
    while (query.next()) {
        PeerInfo info;
        info.id = query.value(0).toString();
        info.displayName = query.value(1).toString();
        const QString addr = query.value(2).toString();
        if (!addr.isEmpty()) {
            info.address = QHostAddress(addr);
        }
        info.listenPort = static_cast<quint16>(query.value(3).toInt());
        const qint64 ts = query.value(4).toLongLong();
        if (ts > 0) {
            info.lastSeen = QDateTime::fromSecsSinceEpoch(ts).toUTC();
        }
        info.capabilities = query.value(5).toString();
        if (!info.id.isEmpty()) {
            list.append(info);
        }
    }
    return list;
}

QSqlDatabase StorageManager::connection() const {
    return QSqlDatabase::database(m_connectionName);
}

void StorageManager::ensureSchema(QSqlDatabase &db) const {
    QSqlQuery query(db);
    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS app_identity (\
        id TEXT PRIMARY KEY,\
        display_name TEXT NOT NULL,\
        listen_port INTEGER NOT NULL,\
        updated_at INTEGER NOT NULL\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS network_subnets (\
        id INTEGER PRIMARY KEY AUTOINCREMENT,\
        network TEXT NOT NULL,\
        prefix INTEGER NOT NULL,\
        type INTEGER NOT NULL\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS general_settings (\
        id INTEGER PRIMARY KEY CHECK (id = 1),\
        auto_start INTEGER NOT NULL DEFAULT 0,\
        lan_update INTEGER NOT NULL DEFAULT 1,\
        reject_shake INTEGER NOT NULL DEFAULT 0,\
        reject_feige_ads INTEGER NOT NULL DEFAULT 0,\
        enable_feige_group INTEGER NOT NULL DEFAULT 1,\
        auto_minimize INTEGER NOT NULL DEFAULT 0,\
        hide_ip_address INTEGER NOT NULL DEFAULT 0,\
        reject_segment_share INTEGER NOT NULL DEFAULT 0,\
        auto_group INTEGER NOT NULL DEFAULT 1,\
        auto_sub_group INTEGER NOT NULL DEFAULT 0,\
        popup_on_message INTEGER NOT NULL DEFAULT 0,\
        flash_on_message INTEGER NOT NULL DEFAULT 1,\
        close_behavior TEXT NOT NULL DEFAULT 'taskbar',\
        friend_display TEXT NOT NULL DEFAULT 'signature'\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS network_settings (\
        id INTEGER PRIMARY KEY CHECK(id = 1),\
        search_port INTEGER NOT NULL DEFAULT 9011,\
        organization_code TEXT,\
        enable_interop INTEGER NOT NULL DEFAULT 0,\
        interop_port INTEGER NOT NULL DEFAULT 2425,\
        bind_interface INTEGER NOT NULL DEFAULT 0,\
        interface_id TEXT,\
        restrict_listed INTEGER NOT NULL DEFAULT 0,\
        auto_refresh INTEGER NOT NULL DEFAULT 1,\
        refresh_interval INTEGER NOT NULL DEFAULT 5\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS notification_settings (\
        id INTEGER PRIMARY KEY CHECK(id = 1),\
        notify_self_online INTEGER NOT NULL DEFAULT 0,\
        notify_peer_online INTEGER NOT NULL DEFAULT 0,\
        notify_incoming INTEGER NOT NULL DEFAULT 1,\
        notify_file_transfer INTEGER NOT NULL DEFAULT 1,\
        notify_mail INTEGER NOT NULL DEFAULT 0,\
        mute_all INTEGER NOT NULL DEFAULT 0,\
        play_group_sound INTEGER NOT NULL DEFAULT 0\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS hotkey_settings (\
        id INTEGER PRIMARY KEY CHECK(id = 1),\
        toggle_main_window INTEGER NOT NULL DEFAULT 1,\
        toggle_shortcut TEXT NOT NULL DEFAULT 'Ctrl + I',\
        capture_enabled INTEGER NOT NULL DEFAULT 1,\
        capture_shortcut TEXT NOT NULL DEFAULT 'Ctrl + Alt + X',\
        lock_enabled INTEGER NOT NULL DEFAULT 1,\
        lock_shortcut TEXT NOT NULL DEFAULT 'Ctrl + L',\
        open_storage_enabled INTEGER NOT NULL DEFAULT 1,\
        open_storage_shortcut TEXT NOT NULL DEFAULT 'Ctrl + R'\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS security_settings (\
        id INTEGER PRIMARY KEY CHECK(id = 1),\
        lock_enabled INTEGER NOT NULL DEFAULT 0,\
        password_hash TEXT,\
        idle_minutes INTEGER NOT NULL DEFAULT 5,\
        idle_action INTEGER NOT NULL DEFAULT 0\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS mail_settings (\
        id INTEGER PRIMARY KEY CHECK(id = 1),\
        address TEXT,\
        imap_server TEXT,\
        smtp_server TEXT,\
        port INTEGER NOT NULL DEFAULT 465,\
        play_sound INTEGER NOT NULL DEFAULT 0,\
        popup_on_mail INTEGER NOT NULL DEFAULT 0\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS shared_directories (\
        id INTEGER PRIMARY KEY AUTOINCREMENT,\
        path TEXT NOT NULL UNIQUE\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS profile_details (\
        id INTEGER PRIMARY KEY CHECK(id = 1),\
        name TEXT,\
        signature TEXT,\
        gender TEXT,\
        avatar_path TEXT,\
        unit TEXT,\
        department TEXT,\
        phone TEXT,\
        mobile TEXT,\
        email TEXT,\
        version TEXT,\
        ip TEXT\
    )"));

    QSqlRecord profileRecord = db.record(QStringLiteral("profile_details"));
    if (profileRecord.indexOf(QStringLiteral("avatar_path")) == -1) {
        query.exec(QStringLiteral("ALTER TABLE profile_details ADD COLUMN avatar_path TEXT"));
    }

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS app_state (\
        id INTEGER PRIMARY KEY CHECK(id = 1),\
        active_role_id TEXT,\
        signature_text TEXT\
    )"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS chat_messages (\
        id INTEGER PRIMARY KEY AUTOINCREMENT,\
        peer_id TEXT NOT NULL,\
        role_name TEXT,\
        message_type TEXT NOT NULL,\
        content TEXT,\
        attachment_path TEXT,\
        outgoing INTEGER NOT NULL,\
        created_at INTEGER NOT NULL\
    )"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_chat_messages_peer ON chat_messages(peer_id, created_at DESC)"));

    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS known_peers (\
        peer_id TEXT PRIMARY KEY,\
        display_name TEXT,\
        address TEXT,\
        listen_port INTEGER,\
        last_seen INTEGER,\
        capabilities TEXT\
    )"));
}

void StorageManager::writeGeneralSettings(const AppSettings &settings, QSqlDatabase &db) const {
    QSqlQuery query(db);
    query.prepare(QStringLiteral("REPLACE INTO general_settings(id, auto_start, lan_update, reject_shake,"
                                 " reject_feige_ads, enable_feige_group, auto_minimize, hide_ip_address,"
                                 " reject_segment_share, auto_group, auto_sub_group, popup_on_message, flash_on_message,"
                                 " close_behavior, friend_display) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    const GeneralSettings &general = settings.general;
    query.addBindValue(boolToInt(general.autoStart));
    query.addBindValue(boolToInt(general.enableLanUpdate));
    query.addBindValue(boolToInt(general.rejectShake));
    query.addBindValue(boolToInt(general.rejectFeigeAds));
    query.addBindValue(boolToInt(general.enableFeigeGroup));
    query.addBindValue(boolToInt(general.autoMinimize));
    query.addBindValue(boolToInt(general.hideIpAddress));
    query.addBindValue(boolToInt(general.rejectSegmentShare));
    query.addBindValue(boolToInt(general.enableAutoGroup));
    query.addBindValue(boolToInt(general.enableAutoSubGroup));
    query.addBindValue(boolToInt(general.popupOnMessage));
    query.addBindValue(boolToInt(general.flashOnMessage));
    query.addBindValue(general.closeBehavior);
    query.addBindValue(general.friendDisplay);
    query.exec();
}

void StorageManager::writeNetworkSettings(const AppSettings &settings, QSqlDatabase &db) const {
    QSqlQuery query(db);
    const NetworkSettings &network = settings.network;
    query.prepare(QStringLiteral("REPLACE INTO network_settings(id, search_port, organization_code, enable_interop,"
                                 " interop_port, bind_interface, interface_id, restrict_listed, auto_refresh,"
                                 " refresh_interval) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(static_cast<int>(network.searchPort));
    query.addBindValue(network.organizationCode);
    query.addBindValue(boolToInt(network.enableInterop));
    query.addBindValue(static_cast<int>(network.interopPort));
    query.addBindValue(boolToInt(network.bindNetworkInterface));
    query.addBindValue(network.boundInterfaceId);
    query.addBindValue(boolToInt(network.restrictToListedSubnets));
    query.addBindValue(boolToInt(network.autoRefresh));
    query.addBindValue(network.refreshIntervalMinutes);
    query.exec();
}

void StorageManager::writeNotificationSettings(const AppSettings &settings, QSqlDatabase &db) const {
    QSqlQuery query(db);
    const NotificationSettings &notifications = settings.notifications;
    query.prepare(QStringLiteral("REPLACE INTO notification_settings(id, notify_self_online, notify_peer_online,"
                                 " notify_incoming, notify_file_transfer, notify_mail, mute_all, play_group_sound)"
                                 " VALUES (1, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(boolToInt(notifications.notifySelfOnline));
    query.addBindValue(boolToInt(notifications.notifyPeerOnline));
    query.addBindValue(boolToInt(notifications.notifyIncomingMessage));
    query.addBindValue(boolToInt(notifications.notifyFileTransfer));
    query.addBindValue(boolToInt(notifications.notifyMailChange));
    query.addBindValue(boolToInt(notifications.muteAll));
    query.addBindValue(boolToInt(notifications.playGroupSound));
    query.exec();
}

void StorageManager::writeHotkeySettings(const AppSettings &settings, QSqlDatabase &db) const {
    QSqlQuery query(db);
    const HotkeySettings &hotkeys = settings.hotkeys;
    query.prepare(QStringLiteral("REPLACE INTO hotkey_settings(id, toggle_main_window, toggle_shortcut,"
                                 " capture_enabled, capture_shortcut, lock_enabled, lock_shortcut,"
                                 " open_storage_enabled, open_storage_shortcut) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(boolToInt(hotkeys.toggleMainWindow));
    query.addBindValue(hotkeys.toggleShortcut);
    query.addBindValue(boolToInt(hotkeys.captureEnabled));
    query.addBindValue(hotkeys.captureShortcut);
    query.addBindValue(boolToInt(hotkeys.lockEnabled));
    query.addBindValue(hotkeys.lockShortcut);
    query.addBindValue(boolToInt(hotkeys.openStorageEnabled));
    query.addBindValue(hotkeys.openStorageShortcut);
    query.exec();
}

void StorageManager::writeSecuritySettings(const AppSettings &settings, QSqlDatabase &db) const {
    QSqlQuery query(db);
    const SecuritySettings &security = settings.security;
    query.prepare(QStringLiteral("REPLACE INTO security_settings(id, lock_enabled, password_hash, idle_minutes,"
                                 " idle_action) VALUES (1, ?, ?, ?, ?)"));
    query.addBindValue(boolToInt(security.lockEnabled));
    query.addBindValue(security.passwordHash);
    query.addBindValue(security.idleMinutes);
    query.addBindValue(security.idleAction == SecuritySettings::LockApplication ? 1 : 0);
    query.exec();
}

void StorageManager::writeMailSettings(const AppSettings &settings, QSqlDatabase &db) const {
    QSqlQuery query(db);
    const MailSettings &mail = settings.mail;
    query.prepare(QStringLiteral("REPLACE INTO mail_settings(id, address, imap_server, smtp_server, port,"
                                 " play_sound, popup_on_mail) VALUES (1, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(mail.address);
    query.addBindValue(mail.imapServer);
    query.addBindValue(mail.smtpServer);
    query.addBindValue(mail.port);
    query.addBindValue(boolToInt(mail.playSoundOnMail));
    query.addBindValue(boolToInt(mail.popupOnMail));
    query.exec();
}

void StorageManager::writeProfile(const AppSettings &settings, QSqlDatabase &db) const {
    QSqlQuery query(db);
    const ProfileDetails &profile = settings.profile;
    query.prepare(QStringLiteral("REPLACE INTO profile_details(id, name, signature, gender, avatar_path, unit, department, phone,"
                                 " mobile, email, version, ip) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(profile.name);
    query.addBindValue(profile.signature);
    query.addBindValue(profile.gender);
    query.addBindValue(profile.avatarPath);
    query.addBindValue(profile.unit);
    query.addBindValue(profile.department);
    query.addBindValue(profile.phone);
    query.addBindValue(profile.mobile);
    query.addBindValue(profile.email);
    query.addBindValue(profile.version);
    query.addBindValue(profile.ip);
    query.exec();
}

void StorageManager::writeSharedDirectories(const QStringList &paths, QSqlDatabase &db) const {
    QSqlQuery query(db);
    query.exec(QStringLiteral("DELETE FROM shared_directories"));
    for (const QString &path : paths) {
        if (path.isEmpty()) {
            continue;
        }
        query.prepare(QStringLiteral("INSERT OR IGNORE INTO shared_directories(path) VALUES (?)"));
        query.addBindValue(path);
        query.exec();
    }
}

void StorageManager::writeSubnets(const QList<QPair<QHostAddress, int>> &subnets, bool blocked, QSqlDatabase &db) const {
    QSqlQuery query(db);
    for (const auto &pair : subnets) {
        if (pair.first.isNull() || pair.second < 0) {
            continue;
        }
        query.prepare(QStringLiteral("INSERT INTO network_subnets(network, prefix, type) VALUES (?, ?, ?)"));
        query.addBindValue(pair.first.toString());
        query.addBindValue(pair.second);
        query.addBindValue(blocked ? 1 : 0);
        query.exec();
    }
}
