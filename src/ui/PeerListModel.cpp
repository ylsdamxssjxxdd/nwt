#include "PeerListModel.h"

PeerListModel::PeerListModel(PeerDirectory *directory, QObject *parent)
    : QAbstractListModel(parent), m_directory(directory) {
    if (m_directory) {
        connect(m_directory, &PeerDirectory::peerListChanged, this, [this]() {
            beginResetModel();
            endResetModel();
        });
    }
}

int PeerListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || !m_directory) {
        return 0;
    }
    return m_directory->peerCount();
}

QVariant PeerListModel::data(const QModelIndex &index, int role) const {
    if (!m_directory || !index.isValid() || index.row() < 0 || index.row() >= m_directory->peerCount()) {
        return {};
    }
    const PeerInfo peer = m_directory->peerAt(index.row());
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

QHash<int, QByteArray> PeerListModel::roleNames() const {
    return {
        {IdRole, "peerId"},
        {DisplayNameRole, "displayName"},
        {AddressRole, "address"},
        {StatusRole, "status"}
    };
}

