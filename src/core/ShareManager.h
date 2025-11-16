#pragma once

#include "ShareTypes.h"

#include <QByteArray>
#include <QHash>
#include <QStringList>

/*!
 * \brief ShareManager 负责处理本地分享目录扫描、远程分享缓存以及收/发文件时的公共逻辑。
 */
class ShareManager {
public:
    ShareManager() = default;

    QList<SharedFileInfo> collectLocalShares(const QStringList &directories);
    bool hasLocalEntry(const QString &entryId) const;
    SharedFileInfo localEntry(const QString &entryId) const;

    void updateRemoteCatalog(const QString &peerId, const QList<SharedFileInfo> &files);
    QList<SharedFileInfo> remoteCatalog(const QString &peerId) const;

    QString saveIncomingFile(const QString &fileName, const QByteArray &data, QString *errorString = nullptr) const;

private:
    QString buildShareEntryId(const QString &filePath) const;

    QHash<QString, SharedFileInfo> m_localShareIndex;
    QHash<QString, QList<SharedFileInfo>> m_remoteCatalogs;
};

