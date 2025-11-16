#pragma once

#include "core/PeerDirectory.h"

#include <QAbstractListModel>

/*!
 * \brief PeerListModel 将核心中的 PeerDirectory 数据包装成 Qt 模型，供 UI 视图绑定。
 */
class PeerListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles { IdRole = Qt::UserRole + 1, DisplayNameRole, AddressRole, StatusRole };
    Q_ENUM(Roles)

    explicit PeerListModel(PeerDirectory *directory, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    PeerDirectory *m_directory = nullptr;
};

