#include "ShareCenterDialog.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ShareCenterDialog::ShareCenterDialog(ChatController *controller, QWidget *parent)
    : QDialog(parent), m_controller(controller) {
    setWindowTitle(tr("我的共享"));
    resize(600, 420);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *localLabel = new QLabel(tr("本地共享目录"), this);
    layout->addWidget(localLabel);
    m_localList = new QListWidget(this);
    layout->addWidget(m_localList, 1);

    auto *localButtons = new QHBoxLayout();
    auto *addBtn = new QPushButton(tr("添加目录"), this);
    auto *removeBtn = new QPushButton(tr("移除目录"), this);
    localButtons->addWidget(addBtn);
    localButtons->addWidget(removeBtn);
    localButtons->addStretch();
    layout->addLayout(localButtons);

    auto *remoteLabel = new QLabel(tr("远程共享文件"), this);
    layout->addWidget(remoteLabel);
    m_remoteList = new QListWidget(this);
    layout->addWidget(m_remoteList, 1);

    auto *remoteButtons = new QHBoxLayout();
    auto *shareBtn = new QPushButton(tr("发送本地共享"), this);
    auto *requestBtn = new QPushButton(tr("请求远端列表"), this);
    auto *downloadBtn = new QPushButton(tr("下载选中项"), this);
    remoteButtons->addWidget(shareBtn);
    remoteButtons->addWidget(requestBtn);
    remoteButtons->addWidget(downloadBtn);
    remoteButtons->addStretch();
    layout->addLayout(remoteButtons);

    connect(addBtn, &QPushButton::clicked, this, &ShareCenterDialog::addDirectory);
    connect(removeBtn, &QPushButton::clicked, this, &ShareCenterDialog::removeDirectory);
    connect(shareBtn, &QPushButton::clicked, this, &ShareCenterDialog::sendLocalShare);
    connect(requestBtn, &QPushButton::clicked, this, &ShareCenterDialog::requestRemoteShare);
    connect(downloadBtn, &QPushButton::clicked, this, &ShareCenterDialog::downloadSelectedShare);

    if (m_controller) {
        connect(m_controller, &ChatController::preferencesChanged, this, [this]() { refreshLocalDirectories(); });
    }
    refreshLocalDirectories();
}

void ShareCenterDialog::setPeerId(const QString &peerId) {
    m_peerId = peerId;
}

void ShareCenterDialog::setRemoteEntries(const QList<SharedFileInfo> &entries) {
    m_remoteEntries = entries;
    refreshRemoteList();
}

void ShareCenterDialog::addDirectory() {
    if (!m_controller) {
        return;
    }
    const QString directory = QFileDialog::getExistingDirectory(this, tr("选择共享目录"));
    if (directory.isEmpty()) {
        return;
    }
    QStringList dirs = m_controller->settings().sharedDirectories;
    if (!dirs.contains(directory)) {
        dirs.append(directory);
        m_controller->updateSharedDirectories(dirs);
    }
}

void ShareCenterDialog::removeDirectory() {
    if (!m_controller || !m_localList) {
        return;
    }
    QListWidgetItem *item = m_localList->currentItem();
    if (!item) {
        return;
    }
    QStringList dirs = m_controller->settings().sharedDirectories;
    dirs.removeAll(item->text());
    m_controller->updateSharedDirectories(dirs);
}

void ShareCenterDialog::sendLocalShare() {
    if (!m_controller || m_peerId.isEmpty()) {
        QMessageBox::information(this, tr("请选择联系人"), tr("需要先在主界面选择一个联系人。"));
        return;
    }
    m_controller->shareCatalogToPeer(m_peerId);
}

void ShareCenterDialog::requestRemoteShare() {
    if (!m_controller || m_peerId.isEmpty()) {
        QMessageBox::information(this, tr("请选择联系人"), tr("需要先在主界面选择一个联系人。"));
        return;
    }
    m_controller->requestPeerShareList(m_peerId);
}

void ShareCenterDialog::downloadSelectedShare() {
    if (!m_controller || m_peerId.isEmpty() || !m_remoteList) {
        return;
    }
    QListWidgetItem *item = m_remoteList->currentItem();
    if (!item) {
        return;
    }
    const QString entryId = item->data(Qt::UserRole).toString();
    if (entryId.isEmpty()) {
        return;
    }
    m_controller->requestPeerSharedFile(m_peerId, entryId);
}

void ShareCenterDialog::refreshLocalDirectories() {
    if (!m_localList || !m_controller) {
        return;
    }
    m_localList->clear();
    for (const QString &path : m_controller->settings().sharedDirectories) {
        m_localList->addItem(path);
    }
}

void ShareCenterDialog::refreshRemoteList() {
    if (!m_remoteList) {
        return;
    }
    m_remoteList->clear();
    for (const SharedFileInfo &info : m_remoteEntries) {
        auto *item =
            new QListWidgetItem(QStringLiteral("%1 (%2 KB)").arg(info.name).arg(info.size / 1024 + 1), m_remoteList);
        item->setData(Qt::UserRole, info.entryId);
    }
}
