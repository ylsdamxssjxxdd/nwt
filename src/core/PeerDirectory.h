#pragma once

#include "PeerInfo.h"

#include <QAbstractListModel>
#include <QVector>

class PeerDirectory : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles { IdRole = Qt::UserRole + 1, DisplayNameRole, AddressRole, StatusRole };
    Q_ENUM(Roles)

    explicit PeerDirectory(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QList<PeerInfo> peers() const;

public slots:
    void upsertPeer(const PeerInfo &info);
    void removePeer(const QString &peerId);

signals:
    void peerListChanged();

private:
    int indexOf(const QString &peerId, const QHostAddress &address) const;

    QVector<PeerInfo> m_peers;
};
