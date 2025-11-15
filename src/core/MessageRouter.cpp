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

void MessageRouter::sendChatMessage(const PeerInfo &peer, const QString &text) {
    if (text.isEmpty()) {
        return;
    }

    QTcpSocket *socket = ensureSession(peer);
    if (!socket) {
        emit routerWarning(tr("无法与 %1 建立会话").arg(peer.displayName));
        return;
    }

    QJsonObject obj{
        {"type", QStringLiteral("chat")},
        {"id", m_localPeerId},
        {"displayName", m_localDisplayName},
        {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"text", text}
    };
    QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    payload.append('\n');
    socket->write(payload);
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
        const QString text = obj.value(QStringLiteral("text")).toString();

        if (!peer.id.isEmpty()) {
            m_socketToPeer.insert(socket, peer.id);
            if (!m_peerSessions.contains(peer.id)) {
                m_peerSessions.insert(peer.id, socket);
            }
        }

        emit messageReceived(peer, text);
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
