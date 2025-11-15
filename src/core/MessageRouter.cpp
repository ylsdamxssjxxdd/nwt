#include "MessageRouter.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>

MessageRouter::MessageRouter(QObject *parent) : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection, this, &MessageRouter::handleNewConnection);
}

bool MessageRouter::startListening(quint16 port) {
    if (m_server.isListening()) {
        m_server.close();
    }

    if (!m_server.listen(QHostAddress::AnyIPv4, port)) {
        emit routerWarning(tr("无法监听 TCP 端口 %1: %2").arg(port).arg(m_server.errorString()));
        return false;
    }
    return true;
}

void MessageRouter::setLocalPeerId(const QString &peerId) {
    m_localPeerId = peerId;
}

void MessageRouter::setLocalDisplayName(const QString &name) {
    m_localDisplayName = name;
}

void MessageRouter::sendChatMessage(const PeerInfo &peer, const QString &text, const QString &roleId,
                                    const QString &roleName) {
    if (text.isEmpty()) {
        return;
    }

    QTcpSocket *socket = ensureSession(peer);
    if (!socket) {
        emit routerWarning(tr("无法与 %1 建立会话").arg(peer.displayName));
        return;
    }

    QJsonObject obj{
        {QStringLiteral("type"), QStringLiteral("chat")},
        {QStringLiteral("id"), m_localPeerId},
        {QStringLiteral("displayName"), m_localDisplayName},
        {QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("text"), text},
        {QStringLiteral("roleId"), roleId},
        {QStringLiteral("roleName"), roleName}
    };
    sendJson(socket, obj);
}

void MessageRouter::sendFilePayload(const PeerInfo &peer, const QString &roleId, const QString &roleName,
                                    const QJsonObject &fileInfo) {
    QTcpSocket *socket = ensureSession(peer);
    if (!socket) {
        emit routerWarning(tr("无法与 %1 建立文件会话").arg(peer.displayName));
        return;
    }

    QJsonObject payload = fileInfo;
    payload.insert(QStringLiteral("type"), QStringLiteral("file"));
    payload.insert(QStringLiteral("id"), m_localPeerId);
    payload.insert(QStringLiteral("displayName"), m_localDisplayName);
    payload.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    payload.insert(QStringLiteral("roleId"), roleId);
    payload.insert(QStringLiteral("roleName"), roleName);
    sendJson(socket, payload);
}

void MessageRouter::sendSharePayload(const PeerInfo &peer, const QJsonObject &payload) {
    QTcpSocket *socket = ensureSession(peer);
    if (!socket) {
        emit routerWarning(tr("无法向 %1 发送共享数据").arg(peer.displayName));
        return;
    }

    QJsonObject object = payload;
    object.insert(QStringLiteral("type"), object.value(QStringLiteral("type")).toString());
    object.insert(QStringLiteral("id"), m_localPeerId);
    object.insert(QStringLiteral("displayName"), m_localDisplayName);
    object.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    sendJson(socket, object);
}

void MessageRouter::handleNewConnection() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        attachSocketSignals(socket);
    }
}

void MessageRouter::readSocket() {
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

    QByteArray &buffer = m_pendingBuffers[socket];
    buffer.append(socket->readAll());

    int newline = buffer.indexOf('\n');
    while (newline != -1) {
        const QByteArray line = buffer.left(newline);
        buffer.remove(0, newline + 1);

        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) {
            newline = buffer.indexOf('\n');
            continue;
        }

        const QJsonObject obj = doc.object();
        PeerInfo peer;
        peer.id = obj.value(QStringLiteral("id")).toString();
        peer.displayName = obj.value(QStringLiteral("displayName")).toString();
        peer.address = socket->peerAddress();
        peer.listenPort = static_cast<quint16>(socket->peerPort());
        peer.lastSeen = QDateTime::currentDateTimeUtc();
        if (!peer.id.isEmpty()) {
            m_socketToPeer.insert(socket, peer.id);
            if (!m_peerSessions.contains(peer.id)) {
                m_peerSessions.insert(peer.id, socket);
            }
        }

        emit messageReceived(peer, obj);
        newline = buffer.indexOf('\n');
    }
}

QTcpSocket *MessageRouter::ensureSession(const PeerInfo &peer) {
    if (peer.id.isEmpty()) {
        return nullptr;
    }

    auto existing = m_peerSessions.value(peer.id);
    if (!existing.isNull() && existing->state() != QAbstractSocket::UnconnectedState) {
        return existing.data();
    }

    auto *socket = new QTcpSocket(this);
    attachSocketSignals(socket);
    socket->connectToHost(peer.address, peer.listenPort);
    m_peerSessions.insert(peer.id, socket);
    m_socketToPeer.insert(socket, peer.id);
    return socket;
}

void MessageRouter::attachSocketSignals(QTcpSocket *socket) {
    connect(socket, &QTcpSocket::readyRead, this, &MessageRouter::readSocket);
    connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
        cleanupSocket(socket);
        socket->deleteLater();
    });
}

void MessageRouter::sendJson(QTcpSocket *socket, const QJsonObject &object) {
    if (!socket) {
        return;
    }
    QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Compact);
    payload.append('\n');
    socket->write(payload);
}

void MessageRouter::cleanupSocket(QTcpSocket *socket) {
    const QString peerId = m_socketToPeer.take(socket);
    if (!peerId.isEmpty()) {
        auto it = m_peerSessions.find(peerId);
        if (it != m_peerSessions.end() && it.value() == socket) {
            m_peerSessions.erase(it);
        }
    }
    m_pendingBuffers.remove(socket);
}
