#include "DiscoveryService.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>

DiscoveryService::DiscoveryService(QObject *parent) : QObject(parent) {
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &DiscoveryService::sendHeartbeat);
}

void DiscoveryService::start(quint16 broadcastPort) {
    m_broadcastPort = broadcastPort;
    if (m_socket.state() == QAbstractSocket::BoundState) {
        m_socket.close();
    }

    if (!m_socket.bind(QHostAddress::AnyIPv4, m_broadcastPort,
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit discoveryWarning(tr("无法绑定 UDP 端口 %1").arg(m_broadcastPort));
        return;
    }

    connect(&m_socket, &QUdpSocket::readyRead, this, &DiscoveryService::readPendingDatagrams);
    m_heartbeatTimer.start(15'000);
    announceOnline();
}

void DiscoveryService::setLocalIdentity(const QString &peerId, const QString &name, quint16 listenPort) {
    m_localId = peerId;
    m_displayName = name;
    m_listenPort = listenPort;
}

void DiscoveryService::setSubnets(const QList<QPair<QHostAddress, int>> &subnets) {
    m_subnets = subnets;
}

void DiscoveryService::probeSubnet(const QHostAddress &network, int prefixLength) {
    const QHostAddress broadcast = broadcastFor(network, prefixLength);
    if (broadcast.isNull()) {
        emit discoveryWarning(tr("子网 %1/%2 无法计算广播地址").arg(network.toString()).arg(prefixLength));
        return;
    }

    const QByteArray payload = buildPacket(QStringLiteral("probe"));
    if (!payload.isEmpty()) {
        m_socket.writeDatagram(payload, broadcast, m_broadcastPort);
    }
}

void DiscoveryService::announceOnline() {
    sendPacket("hello");
}

void DiscoveryService::stop() {
    if (m_heartbeatTimer.isActive()) {
        m_heartbeatTimer.stop();
    }
    disconnect(&m_socket, nullptr, this, nullptr);
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }
}

void DiscoveryService::readPendingDatagrams() {
    while (m_socket.hasPendingDatagrams()) {
        QByteArray payload;
        payload.resize(int(m_socket.pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_socket.readDatagram(payload.data(), payload.size(), &sender, &senderPort);
        Q_UNUSED(senderPort)
        processPacket(payload, sender);
    }
}

void DiscoveryService::sendHeartbeat() {
    sendPacket("heartbeat");
}

void DiscoveryService::sendPacket(const QString &type) {
    if (m_socket.state() != QAbstractSocket::BoundState || m_localId.isEmpty()) {
        return;
    }

    const QByteArray payload = buildPacket(type);
    if (payload.isEmpty()) {
        return;
    }

    const auto targets = broadcastTargets();
    for (const auto &target : targets) {
        m_socket.writeDatagram(payload, target, m_broadcastPort);
    }
}

QByteArray DiscoveryService::buildPacket(const QString &type) const {
    if (m_localId.isEmpty()) {
        return {};
    }

    QJsonObject obj{
        {"type", type},
        {"id", m_localId},
        {"displayName", m_displayName},
        {"listenPort", static_cast<int>(m_listenPort)},
        {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    };
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

void DiscoveryService::processPacket(const QByteArray &payload, const QHostAddress &sender) {
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    const QString senderId = obj.value(QStringLiteral("id")).toString();
    if (senderId.isEmpty() || senderId == m_localId) {
        return;
    }

    PeerInfo info;
    info.id = senderId;
    info.displayName = obj.value(QStringLiteral("displayName")).toString();
    info.address = sender;
    info.listenPort = static_cast<quint16>(obj.value(QStringLiteral("listenPort")).toInt());
    info.lastSeen = QDateTime::currentDateTimeUtc();
    info.capabilities = obj.value(QStringLiteral("capabilities")).toString();

    emit peerDiscovered(info);
}

QList<QHostAddress> DiscoveryService::broadcastTargets() const {
    QList<QHostAddress> targets;

    if (!m_subnets.isEmpty()) {
        for (const auto &subnet : m_subnets) {
            const QHostAddress address = broadcastFor(subnet.first, subnet.second);
            if (!address.isNull()) {
                targets.append(address);
            }
        }
        return targets;
    }

    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)) {
            continue;
        }
        for (const auto &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) {
                continue;
            }
            const QHostAddress broadcast = entry.broadcast();
            if (!broadcast.isNull()) {
                targets.append(broadcast);
            }
        }
    }

    if (targets.isEmpty()) {
        targets.append(QHostAddress::Broadcast);
    }
    return targets;
}

QHostAddress DiscoveryService::broadcastFor(const QHostAddress &network, int prefixLength) {
    if (network.protocol() != QAbstractSocket::IPv4Protocol || prefixLength < 0 || prefixLength > 32) {
        return {};
    }

    const quint32 addr = network.toIPv4Address();
    const quint32 mask = prefixLength == 0 ? 0 : 0xFFFFFFFFu << (32 - prefixLength);
    const quint32 broadcast = (addr & mask) | (~mask);
    return QHostAddress(broadcast);
}
