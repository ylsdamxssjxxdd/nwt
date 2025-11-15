#include "PeerDirectory.h"

#include <QDateTime>

PeerDirectory::PeerDirectory(QObject *parent) : QAbstractListModel(parent) {}

int PeerDirectory::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_peers.size();
}

QVariant PeerDirectory::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_peers.size()) {
        return {};
    }

    const PeerInfo &peer = m_peers.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case DisplayNameRole:
        return peer.displayName.isEmpty() ? peer.id : peer.displayName;
    case IdRole:
        return peer.id;
    case AddressRole:
        return QStringLiteral("%1:%2").arg(peer.address.toString()).arg(peer.listenPort);
    case StatusRole:
        return peer.lastSeen.toString(Qt::ISODate);
    default:
        return {};
    }
}

QHash<int, QByteArray> PeerDirectory::roleNames() const {
    return {
        {IdRole, "peerId"},
        {DisplayNameRole, "displayName"},
        {AddressRole, "address"},
        {StatusRole, "status"}
    };
}

QList<PeerInfo> PeerDirectory::peers() const {
    return m_peers.toList();
}

void PeerDirectory::upsertPeer(const PeerInfo &info) {
    int index = indexOf(info.id, info.address);
    if (index >= 0) {
        m_peers[index] = info;
        emit dataChanged(this->index(index), this->index(index));
    } else {
        const int row = m_peers.size();
        beginInsertRows(QModelIndex(), row, row);
        m_peers.append(info);
        endInsertRows();
    }
    emit peerListChanged();
}

void PeerDirectory::removePeer(const QString &peerId) {
    for (int row = 0; row < m_peers.size(); ++row) {
        if (m_peers.at(row).id == peerId) {
            beginRemoveRows(QModelIndex(), row, row);
            m_peers.removeAt(row);
            endRemoveRows();
            emit peerListChanged();
            break;
        }
    }
}

int PeerDirectory::indexOf(const QString &peerId, const QHostAddress &address) const {
    for (int i = 0; i < m_peers.size(); ++i) {
        const PeerInfo &candidate = m_peers.at(i);
        if (candidate.id == peerId && candidate.address == address) {
            return i;
        }
    }
    return -1;
}
