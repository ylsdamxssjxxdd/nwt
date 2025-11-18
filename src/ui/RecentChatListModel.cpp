#include "RecentChatListModel.h"

#include "core/ChatController.h"
#include "core/PeerDirectory.h"

RecentChatListModel::RecentChatListModel(ChatController *controller, QObject *parent)
    : QAbstractListModel(parent), m_controller(controller) {
    refresh();
}

int RecentChatListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant RecentChatListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case DisplayNameRole:
        return entry.displayName;
    case IdRole:
        return entry.peerId;
    default:
        return {};
    }
}

QHash<int, QByteArray> RecentChatListModel::roleNames() const {
    return {
        {IdRole, "peerId"},
        {DisplayNameRole, "displayName"}
    };
}

void RecentChatListModel::refresh() {
    beginResetModel();
    m_entries.clear();
    if (!m_controller) {
        endResetModel();
        return;
    }

    const QVector<QString> peerIds = m_controller->recentPeerIds(100);
    PeerDirectory *directory = m_controller->peerDirectory();

    for (const QString &peerId : peerIds) {
        if (peerId.isEmpty()) {
            continue;
        }

        QString displayName;
        // 1. 优先使用联系人目录中的显示名称（如果存在且非空）
        if (directory) {
            const int count = directory->peerCount();
            for (int i = 0; i < count; ++i) {
                const PeerInfo peer = directory->peerAt(i);
                if (peer.id == peerId) {
                    if (!peer.displayName.isEmpty()) {
                        displayName = peer.displayName;
                    } else {
                        displayName = peer.id;
                    }
                    break;
                }
            }
        }

        // 2. 若仍然没有可读名称，则尝试从最近一条消息中拿“角色名”
        if (displayName.isEmpty()) {
            const QVector<StoredMessage> history = m_controller->recentMessages(peerId, 1);
            if (!history.isEmpty()) {
                const StoredMessage &last = history.first();
                // 仅在为对方发来的消息时使用角色名，避免把本机名称当作标题
                if (last.direction == MessageDirection::Incoming && !last.roleName.isEmpty()) {
                    displayName = last.roleName;
                }
            }
        }

        // 3. 兜底回退为 peerId（避免空字符串）
        if (displayName.isEmpty()) {
            displayName = peerId;
        }

        m_entries.append({peerId, displayName});
    }

    endResetModel();
}
