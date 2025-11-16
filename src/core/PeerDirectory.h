#pragma once

#include "PeerInfo.h"

#include <QObject>
#include <QList>
#include <QVector>

class PeerDirectory : public QObject {
    Q_OBJECT

public:
    explicit PeerDirectory(QObject *parent = nullptr);

    QList<PeerInfo> peers() const;
    PeerInfo peerAt(int index) const;
    int peerCount() const;

public slots:
    void upsertPeer(const PeerInfo &info);
    void removePeer(const QString &peerId);

signals:
    void peerListChanged();

private:
    int indexOf(const QString &peerId, const QHostAddress &address) const;

    QVector<PeerInfo> m_peers;
};
