#pragma once

#include "PeerInfo.h"

#include <QHostAddress>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QUdpSocket>

class DiscoveryService : public QObject {
    Q_OBJECT

public:
    explicit DiscoveryService(QObject *parent = nullptr);

    void start(quint16 broadcastPort = 45454);
    void setLocalIdentity(const QString &peerId, const QString &name, quint16 listenPort);
    void setSubnets(const QList<QPair<QHostAddress, int>> &subnets);
    void probeSubnet(const QHostAddress &network, int prefixLength);
    void announceOnline();
    void stop();

signals:
    void peerDiscovered(const PeerInfo &info);
    void discoveryWarning(const QString &message);

private slots:
    void readPendingDatagrams();
    void sendHeartbeat();

private:
    void sendPacket(const QString &type);
    QByteArray buildPacket(const QString &type) const;
    void processPacket(const QByteArray &payload, const QHostAddress &sender);
    QList<QHostAddress> broadcastTargets() const;
    static QHostAddress broadcastFor(const QHostAddress &network, int prefixLength);

    QUdpSocket m_socket;
    QTimer m_heartbeatTimer;
    QString m_localId;
    QString m_displayName;
    quint16 m_listenPort = 0;
    quint16 m_broadcastPort = 45454;
    QList<QPair<QHostAddress, int>> m_subnets;
};
