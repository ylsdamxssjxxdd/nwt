#include "PeerDirectory.h"

PeerDirectory::PeerDirectory(QObject *parent) : QObject(parent) {}

QList<PeerInfo> PeerDirectory::peers() const {
    return m_peers.toList();
}

PeerInfo PeerDirectory::peerAt(int index) const {
    if (index < 0 || index >= m_peers.size()) {
        return {};
    }
    return m_peers.at(index);
}

int PeerDirectory::peerCount() const {
    return m_peers.size();
}

void PeerDirectory::upsertPeer(const PeerInfo &info) {
    int index = indexOf(info.id, info.address);
    if (index >= 0) {
        m_peers[index] = info;
    } else {
        m_peers.append(info);
    }
    emit peerListChanged();
}

void PeerDirectory::removePeer(const QString &peerId) {
    for (int row = 0; row < m_peers.size(); ++row) {
        if (m_peers.at(row).id == peerId) {
            m_peers.removeAt(row);
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
