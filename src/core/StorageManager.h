#pragma once

#include "SettingsTypes.h"

#include <QHostAddress>
#include <QList>
#include <QPair>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

class PeerInfo;

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
    /*!
     * \brief recentPeerIds 返回按照最近消息时间倒序排序的对话联系人 ID 列表。
     * \param limit 最多返回的联系人数量上限
     * \return 去重后的联系人 ID 列表，最近有消息往来的联系人排在前面
     */
    QVector<QString> recentPeerIds(int limit = 100) const;

    /*!
     * \brief upsertKnownPeer 将发现到的联系人写入或更新到“已知联系人”表。
     * \param peer 当前发现到的联系人信息
     */
    void upsertKnownPeer(const PeerInfo &peer);
    /*!
     * \brief knownPeers 读取历史上发现过的联系人列表。
     * \return 已持久化的联系人集合
     */
    QList<PeerInfo> knownPeers() const;

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
