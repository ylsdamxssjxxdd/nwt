#pragma once

#include "DiscoveryService.h"
#include "MessageRouter.h"
#include "PeerDirectory.h"

#include <QHostAddress>
#include <QList>
#include <QPair>

class ChatController : public QObject {
    Q_OBJECT

public:
    explicit ChatController(QObject *parent = nullptr);

    bool initialize();
    PeerDirectory *peerDirectory();
    QString localDisplayName() const;
    quint16 listenPort() const;
    QList<QPair<QHostAddress, int>> configuredSubnets() const;

public slots:
    void sendMessageToPeer(const QString &peerId, const QString &text);
    void addSubnet(const QHostAddress &network, int prefixLength);

signals:
    void chatMessageReceived(const PeerInfo &peer, const QString &text);
    void statusInfo(const QString &text);
    void controllerWarning(const QString &message);

private:
    QString configFilePath() const;
    void loadSettings();
    void persistSettings();
    PeerInfo findPeer(const QString &peerId) const;

    PeerDirectory m_peerDirectory;
    DiscoveryService m_discovery;
    MessageRouter m_router;
    QString m_localId;
    QString m_displayName;
    quint16 m_listenPort = 45600;
    QList<QPair<QHostAddress, int>> m_subnets;
};
