#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QString>

class ChatController;

/*!
 * \brief RecentChatListModel 提供“最近聊天”标签使用的会话列表模型。
 *
 * 模型数据来源于 StorageManager 中的聊天记录聚合结果，
 * 每一行代表一个曾经有过消息往来的联系人。
 */
class RecentChatListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles { IdRole = Qt::UserRole + 1, DisplayNameRole };
    Q_ENUM(Roles)

    explicit RecentChatListModel(ChatController *controller, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /*!
     * \brief refresh 重新从控制器拉取最近会话列表。
     *
     * 一般在发送/接收消息后调用，用于让“最近聊天”标签即时刷新。
     */
    void refresh();

private:
    struct Entry {
        QString peerId;
        QString displayName;
    };

    ChatController *m_controller = nullptr;
    QVector<Entry> m_entries;
};

