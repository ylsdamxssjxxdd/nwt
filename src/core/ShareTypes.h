#pragma once

#include <QMetaType>
#include <QString>
#include <QtGlobal>

/*!
 * \brief 文件共享条目的元数据，用于描述分享列表中的单个文件。
 */
struct SharedFileInfo {
    QString entryId;
    QString name;
    quint64 size = 0;
    QString filePath;
};
Q_DECLARE_METATYPE(SharedFileInfo)
