#include "ShareManager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

QList<SharedFileInfo> ShareManager::collectLocalShares(const QStringList &directories) {
    QList<SharedFileInfo> files;
    m_localShareIndex.clear();
    for (const QString &path : directories) {
        QDir dir(path);
        if (!dir.exists()) {
            continue;
        }
        const QFileInfoList fileInfos = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        for (const QFileInfo &info : fileInfos) {
            SharedFileInfo entry;
            entry.name = info.fileName();
            entry.size = static_cast<quint64>(info.size());
            entry.filePath = info.absoluteFilePath();
            entry.entryId = buildShareEntryId(entry.filePath);
            files.append(entry);
            m_localShareIndex.insert(entry.entryId, entry);
        }
    }
    return files;
}

bool ShareManager::hasLocalEntry(const QString &entryId) const {
    return m_localShareIndex.contains(entryId);
}

SharedFileInfo ShareManager::localEntry(const QString &entryId) const {
    return m_localShareIndex.value(entryId);
}

void ShareManager::updateRemoteCatalog(const QString &peerId, const QList<SharedFileInfo> &files) {
    m_remoteCatalogs.insert(peerId, files);
}

QList<SharedFileInfo> ShareManager::remoteCatalog(const QString &peerId) const {
    return m_remoteCatalogs.value(peerId);
}

QString ShareManager::saveIncomingFile(const QString &fileName, const QByteArray &data, QString *errorString) const {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("downloads"));
    }

    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QString resolvedName = fileName;
    QString targetPath = dir.filePath(resolvedName);
    int suffix = 1;
    while (QFile::exists(targetPath)) {
        const QFileInfo info(fileName);
        const QString base = info.completeBaseName();
        const QString ext = info.suffix();
        if (ext.isEmpty()) {
            resolvedName = QStringLiteral("%1_%2").arg(base).arg(suffix++);
        } else {
            resolvedName = QStringLiteral("%1_%2.%3").arg(base).arg(suffix++).arg(ext);
        }
        targetPath = dir.filePath(resolvedName);
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorString) {
            *errorString = file.errorString();
        }
        return {};
    }
    file.write(data);
    file.close();
    return targetPath;
}

QString ShareManager::buildShareEntryId(const QString &filePath) const {
    const QByteArray hash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Sha1);
    return QString::fromLatin1(hash.toHex());
}

