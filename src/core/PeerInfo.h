#pragma once

#include <QDateTime>
#include <QHostAddress>
#include <QMetaType>
#include <QString>

struct PeerInfo {
    QString id;
    QString displayName;
    QHostAddress address;
    quint16 listenPort = 0;
    QDateTime lastSeen;
    QString capabilities;
};

Q_DECLARE_METATYPE(PeerInfo)
