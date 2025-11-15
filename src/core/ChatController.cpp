#include "ChatController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <utility>
#include <limits>

ChatController::ChatController(QObject *parent) : QObject(parent) {
    qRegisterMetaType<PeerInfo>("PeerInfo");

    connect(&m_discovery, &DiscoveryService::peerDiscovered, &m_peerDirectory, &PeerDirectory::upsertPeer);
    connect(&m_discovery, &DiscoveryService::discoveryWarning, this, &ChatController::controllerWarning);
    connect(&m_router, &MessageRouter::routerWarning, this, &ChatController::controllerWarning);
    connect(&m_router, &MessageRouter::messageReceived, this, &ChatController::chatMessageReceived);
}

bool ChatController::initialize() {
    loadSettings();

    if (!m_router.startListening(m_listenPort)) {
        emit controllerWarning(tr("消息路由监听失败"));
    }
    m_router.setLocalPeerId(m_localId);
    m_router.setLocalDisplayName(m_displayName);

    m_discovery.setLocalIdentity(m_localId, m_displayName, m_listenPort);
    m_discovery.setSubnets(m_subnets);
    m_discovery.start();
    m_discovery.announceOnline();

    emit statusInfo(tr("本机 ID: %1 端口: %2").arg(m_localId).arg(m_listenPort));
    persistSettings();
    return true;
}

PeerDirectory *ChatController::peerDirectory() {
    return &m_peerDirectory;
}

QString ChatController::localDisplayName() const {
    return m_displayName;
}

quint16 ChatController::listenPort() const {
    return m_listenPort;
}

QList<QPair<QHostAddress, int>> ChatController::configuredSubnets() const {
    return m_subnets;
}

void ChatController::sendMessageToPeer(const QString &peerId, const QString &text) {
    const PeerInfo peer = findPeer(peerId);
    if (peer.id.isEmpty()) {
        emit controllerWarning(tr("未找到联系人 %1").arg(peerId));
        return;
    }
    m_router.sendChatMessage(peer, text);
}

void ChatController::addSubnet(const QHostAddress &network, int prefixLength) {
    const QString token = QStringLiteral("%1/%2").arg(network.toString()).arg(prefixLength);
    for (const auto &pair : std::as_const(m_subnets)) {
        const QString existing = QStringLiteral("%1/%2").arg(pair.first.toString()).arg(pair.second);
        if (existing == token) {
            return;
        }
    }
    m_subnets.append(qMakePair(network, prefixLength));
    m_discovery.setSubnets(m_subnets);
    m_discovery.probeSubnet(network, prefixLength);
    m_discovery.announceOnline();
    persistSettings();
}

QString ChatController::configFilePath() const {
    const QString baseDir = QCoreApplication::applicationDirPath();
    return QDir(baseDir).filePath(QStringLiteral("lan_chat_settings.json"));
}

void ChatController::loadSettings() {
    QFile file(configFilePath());
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const QJsonObject root = doc.object();
        const QJsonObject identity = root.value(QStringLiteral("identity")).toObject();
        const QJsonObject network = root.value(QStringLiteral("network")).toObject();

        m_localId = identity.value(QStringLiteral("id")).toString();
        m_displayName = identity.value(QStringLiteral("name")).toString();
        if (m_displayName.isEmpty()) {
            m_displayName = m_localId;
        }
        const int portValue = network.value(QStringLiteral("listenPort")).toInt(m_listenPort);
        if (portValue > 0 && portValue <= std::numeric_limits<quint16>::max()) {
            m_listenPort = static_cast<quint16>(portValue);
        }

        const QJsonArray subnetArray = network.value(QStringLiteral("subnets")).toArray();
        m_subnets.clear();
        for (const QJsonValue &value : subnetArray) {
            const QJsonObject entry = value.toObject();
            QHostAddress addr(entry.value(QStringLiteral("network")).toString());
            const int prefix = entry.value(QStringLiteral("prefix")).toInt(-1);
            if (!addr.isNull() && prefix >= 0 && prefix <= 32) {
                m_subnets.append(qMakePair(addr, prefix));
            }
        }
    }

    if (m_localId.isEmpty()) {
        m_localId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (m_displayName.isEmpty()) {
        m_displayName = m_localId;
    }
}

void ChatController::persistSettings() {
    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit controllerWarning(tr("无法写入配置: %1").arg(file.errorString()));
        return;
    }

    QJsonObject identity{
        {QStringLiteral("id"), m_localId},
        {QStringLiteral("name"), m_displayName}
    };

    QJsonArray subnetArray;
    for (const auto &pair : std::as_const(m_subnets)) {
        subnetArray.append(QJsonObject{
            {QStringLiteral("network"), pair.first.toString()},
            {QStringLiteral("prefix"), pair.second}
        });
    }

    QJsonObject network{
        {QStringLiteral("listenPort"), static_cast<int>(m_listenPort)},
        {QStringLiteral("subnets"), subnetArray}
    };

    QJsonObject root{
        {QStringLiteral("identity"), identity},
        {QStringLiteral("network"), network}
    };

    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

PeerInfo ChatController::findPeer(const QString &peerId) const {
    for (const auto &peer : m_peerDirectory.peers()) {
        if (peer.id == peerId) {
            return peer;
        }
    }
    return {};
}
