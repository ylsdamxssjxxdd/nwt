#pragma once

#include "PeerInfo.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

class MessageRouter : public QObject {
    Q_OBJECT

public:
    explicit MessageRouter(QObject *parent = nullptr);

    bool startListening(quint16 port);
    void setLocalPeerId(const QString &peerId);
    void setLocalDisplayName(const QString &name);
    void sendChatMessage(const PeerInfo &peer, const QString &text, const QString &roleId, const QString &roleName);
    void sendFilePayload(const PeerInfo &peer, const QString &roleId, const QString &roleName, const QJsonObject &fileInfo);
    void sendSharePayload(const PeerInfo &peer, const QJsonObject &payload);

signals:
    void messageReceived(const PeerInfo &peer, const QJsonObject &payload);
    void routerWarning(const QString &message);

private slots:
    void handleNewConnection();
    void readSocket();

private:
    QTcpSocket *ensureSession(const PeerInfo &peer);
    void attachSocketSignals(QTcpSocket *socket);
    void sendJson(QTcpSocket *socket, const QJsonObject &object);
    void cleanupSocket(QTcpSocket *socket);

    QTcpServer m_server;
    QString m_localPeerId;
    QString m_localDisplayName;
    QHash<QString, QPointer<QTcpSocket>> m_peerSessions;
    QHash<QTcpSocket *, QString> m_socketToPeer;
    QHash<QTcpSocket *, QByteArray> m_pendingBuffers;
};
