#pragma once

#include "SettingsTypes.h"

#include <QHostAddress>
#include <QList>
#include <QPair>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

/*!
 * \brief 聊天消息方向。
 */
enum class MessageDirection { Incoming = 0, Outgoing = 1 };

/*!
 * \brief 数据库中持久化的全局状态。
 */
struct PersistedState {
    QString localId;
    QString displayName;
    quint16 listenPort = 45600;
    QList<QPair<QHostAddress, int>> subnets;
    QList<QPair<QHostAddress, int>> blockedSubnets;
    AppSettings settings;
};

/*!
 * \brief 聊天记录条目。
 */
struct StoredMessage {
    qint64 id = 0;
    QString peerId;
    QString roleName;
    QString messageType;
    QString content;
    QString attachmentPath;
    MessageDirection direction = MessageDirection::Outgoing;
    qint64 timestamp = 0;
};

/*!
 * \brief 管理聊天应用的SQLite数据库，负责配置与消息记录持久化。
 */
class StorageManager {
public:
    StorageManager();
    ~StorageManager();

    bool initialize(const QString &databasePath);
    bool isReady() const;
    QString databasePath() const;

    PersistedState loadState() const;
    void saveState(const PersistedState &state);
    void storeMessage(const StoredMessage &message);
    QVector<StoredMessage> recentMessages(const QString &peerId, int limit = 100) const;

private:
    QSqlDatabase connection() const;
    void ensureSchema(QSqlDatabase &db) const;
    void writeGeneralSettings(const AppSettings &settings, QSqlDatabase &db) const;
    void writeNetworkSettings(const AppSettings &settings, QSqlDatabase &db) const;
    void writeNotificationSettings(const AppSettings &settings, QSqlDatabase &db) const;
    void writeHotkeySettings(const AppSettings &settings, QSqlDatabase &db) const;
    void writeSecuritySettings(const AppSettings &settings, QSqlDatabase &db) const;
    void writeMailSettings(const AppSettings &settings, QSqlDatabase &db) const;
    void writeProfile(const AppSettings &settings, QSqlDatabase &db) const;
    void writeSharedDirectories(const QStringList &paths, QSqlDatabase &db) const;
    void writeSubnets(const QList<QPair<QHostAddress, int>> &subnets, bool blocked, QSqlDatabase &db) const;

    QString m_databasePath;
    QString m_connectionName;
    bool m_initialized = false;
};
