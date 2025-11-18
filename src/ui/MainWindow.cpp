#include "MainWindow.h"

#include "ChatPanel.h"
#include "ContactsSidebar.h"
#include "PeerListModel.h"
#include "RecentChatListModel.h"
#include "ProfileDialog.h"
#include "SettingsDialog.h"
#include "core/LanguageKeys.h"
#include "core/LanguageManager.h"
#include "core/StorageManager.h"

#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QListView>
#include <QStandardItemModel>
#include <algorithm>

MainWindow::MainWindow(ChatController *controller, QWidget *parent)
    : QMainWindow(parent), m_controller(controller) {
    setupUi();
    if (!m_controller) {
        return;
    }

    if (auto *directory = m_controller->peerDirectory()) {
        m_peerListModel = new PeerListModel(directory, this);
        connect(directory, &PeerDirectory::peerListChanged, this, &MainWindow::updatePeerPlaceholder);
    }

    m_recentListModel = new RecentChatListModel(m_controller, this);
    m_groupListModel = new QStandardItemModel(this);

    if (m_peerList) {
        // 默认进入“最近聊天”标签
        m_peerList->setModel(m_recentListModel);
        connect(m_peerList, &QListView::clicked, this, &MainWindow::handlePeerSelection);
    }

    connect(m_chatPanel, &ChatPanel::sendRequested, this, &MainWindow::handleSend);
    connect(m_chatPanel, &ChatPanel::fileSendRequested, this, &MainWindow::handleSendFile);
    connect(m_chatPanel, &ChatPanel::shareCenterRequested, this, &MainWindow::openShareCenter);

    connect(m_contactsSidebar, &ContactsSidebar::settingsRequested, this, &MainWindow::openSettingsDialog);
    connect(m_contactsSidebar, &ContactsSidebar::profileRequested, this, &MainWindow::openProfileDialog);
    connect(m_contactsSidebar, &ContactsSidebar::statusHintRequested, this, &MainWindow::showStatus);

    connect(m_controller, &ChatController::chatMessageReceived, this, &MainWindow::appendMessage);
    connect(m_controller, &ChatController::fileReceived, this, &MainWindow::handleFileReceived);
    connect(m_controller, &ChatController::shareCatalogReceived, this, &MainWindow::handleShareCatalog);
    connect(m_controller, &ChatController::statusInfo, this, &MainWindow::showStatus);
    connect(m_controller, &ChatController::controllerWarning, this, &MainWindow::showStatus);
    connect(m_controller, &ChatController::roleChanged, this, [this]() { refreshProfileCard(); });
    connect(m_controller, &ChatController::profileUpdated, this, [this](const ProfileDetails &) { refreshProfileCard(); });

    if (m_contactsSidebar) {
        connect(m_contactsSidebar, &ContactsSidebar::tabChanged, this, &MainWindow::handleSidebarTabChanged);
    }

    refreshProfileCard();
    updatePeerPlaceholder();
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_contactsSidebar = new ContactsSidebar(central);
    rootLayout->addWidget(m_contactsSidebar, 0);
    m_peerList = m_contactsSidebar->peerListView();

    m_chatPanel = new ChatPanel(central);
    rootLayout->addWidget(m_chatPanel, 1);

    setCentralWidget(central);
    setMinimumSize(960, 600);
    setWindowTitle(LanguageManager::text(LangKey::MainWindow::Title, QStringLiteral("内网通 - 连接每一位同伴")));
}

void MainWindow::handleSidebarTabChanged(int index) {
    if (!m_peerList || !m_controller) {
        return;
    }

    switch (index) {
    case ContactsSidebar::RecentTab:
        if (m_recentListModel) {
            m_recentListModel->refresh();
            m_peerList->setModel(m_recentListModel);
        }
        break;
    case ContactsSidebar::ContactsTab:
        if (m_peerListModel) {
            m_peerList->setModel(m_peerListModel);
        }
        break;
    case ContactsSidebar::GroupsTab:
        if (m_groupListModel) {
            m_peerList->setModel(m_groupListModel);
        }
        break;
    default:
        break;
    }
    updatePeerPlaceholder();
}

void MainWindow::updatePeerPlaceholder() {
    bool hasItems = false;
    if (m_peerList && m_peerList->model()) {
        hasItems = m_peerList->model()->rowCount() > 0;
    }
    if (m_contactsSidebar) {
        m_contactsSidebar->setPeerPlaceholderVisible(hasItems);
    }
}

void MainWindow::refreshProfileCard() {
    if (!m_controller || !m_contactsSidebar) {
        return;
    }
    const ProfileDetails profile = m_controller->profileDetails();
    const QString displayName = profile.name.isEmpty() ? m_controller->localDisplayName() : profile.name;
    const QString signature = profile.signature.isEmpty()
                                  ? LanguageManager::text(LangKey::ProfileCard::EditSignature, QStringLiteral("编辑我的签名"))
                                  : profile.signature;
    m_contactsSidebar->setProfileInfo(displayName, signature);
    m_contactsSidebar->setAvatarLetter(displayName.left(1));
}

void MainWindow::updateChatHeader(const QString &displayName) {
    if (m_chatPanel) {
        m_chatPanel->setChatHeader(displayName, QString());
    }
}

void MainWindow::loadConversation(const QString &peerId, const QString &peerName) {
    if (!m_chatPanel) {
        return;
    }
    m_chatPanel->resetConversation();
    if (!m_controller || peerId.isEmpty()) {
        return;
    }
    const auto history = m_controller->recentMessages(peerId, 200);
    if (history.isEmpty()) {
        return;
    }
    const QString peerLabel = peerName.isEmpty() ? peerId : peerName;
    const ProfileDetails profile = m_controller->profileDetails();
    const QString localLabel = profile.name.isEmpty() ? m_controller->localDisplayName() : profile.name;
    for (int i = history.size() - 1; i >= 0; --i) {
        const StoredMessage &msg = history.at(i);
        const QDateTime time = QDateTime::fromSecsSinceEpoch(msg.timestamp).toLocalTime();
        const QString timeText = time.toString(QStringLiteral("HH:mm:ss"));
        const QString speaker = msg.roleName.isEmpty()
                                    ? (msg.direction == MessageDirection::Outgoing ? localLabel : peerLabel)
                                    : msg.roleName;
        const QString content = msg.content;
        m_chatPanel->appendTimelineHint(timeText, QString());
        if (msg.direction == MessageDirection::Outgoing) {
            m_chatPanel->appendOutgoingMessage(timeText, speaker, content);
        } else {
            m_chatPanel->appendIncomingMessage(timeText, speaker, content);
        }
    }
}

void MainWindow::handleSend() {
    if (!m_controller || !m_chatPanel) {
        return;
    }

    const QString text = m_chatPanel->inputText();
    if (text.isEmpty()) {
        return;
    }
    if (m_currentPeerId.isEmpty()) {
        showStatus(LanguageManager::text(LangKey::MainWindow::SelectContactHint, QStringLiteral("请先选择联系人")));
        return;
    }

    m_controller->sendMessageToPeer(m_currentPeerId, text);
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const ProfileDetails profile = m_controller->profileDetails();
    const QString roleDisplay =
        profile.name.isEmpty()
            ? LanguageManager::text(LangKey::MainWindow::DefaultRoleName, QStringLiteral("我"))
            : profile.name;
    m_chatPanel->appendTimelineHint(timestamp, QString());
    m_chatPanel->appendOutgoingMessage(timestamp, roleDisplay, text);
    m_chatPanel->clearInput();
    m_chatPanel->focusInput();
    if (m_recentListModel) {
        m_recentListModel->refresh();
    }
    updatePeerPlaceholder();
}

void MainWindow::handleSendFile() {
    if (!m_controller) {
        return;
    }
    if (m_currentPeerId.isEmpty()) {
        showStatus(LanguageManager::text(LangKey::MainWindow::SelectContactHint, QStringLiteral("请先选择联系人")));
        return;
    }
    const QString filePath = QFileDialog::getOpenFileName(
        this, LanguageManager::text(LangKey::MainWindow::FileDialogTitle, QStringLiteral("选择要发送的文件")));
    if (filePath.isEmpty()) {
        return;
    }
    m_controller->sendFileToPeer(m_currentPeerId, filePath);
}

void MainWindow::handlePeerSelection(const QModelIndex &index) {
    if (!index.isValid()) {
        return;
    }

    m_currentPeerId = index.data(PeerListModel::IdRole).toString();
    const QString display = index.data(Qt::DisplayRole).toString();
    showStatus(
        LanguageManager::text(LangKey::MainWindow::FileSelected, QStringLiteral("已选中 %1")).arg(display));
    updateChatHeader(display);
    loadConversation(m_currentPeerId, display);
    if (m_shareDialog) {
        m_shareDialog->setPeerId(m_currentPeerId);
        m_shareDialog->setRemoteEntries(m_remoteShares.value(m_currentPeerId));
    }
}

void MainWindow::appendMessage(const PeerInfo &peer, const QString &roleName, const QString &text) {
    if (!m_chatPanel) {
        return;
    }
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString label = peer.displayName.isEmpty() ? peer.id : peer.displayName;
    const QString speaker = roleName.isEmpty() ? label : QStringLiteral("%1(%2)").arg(label, roleName);
    m_chatPanel->appendTimelineHint(timestamp, QString());
    m_chatPanel->appendIncomingMessage(timestamp, speaker, text);
    if (m_recentListModel) {
        m_recentListModel->refresh();
    }
    updatePeerPlaceholder();
}

void MainWindow::showStatus(const QString &text) {
    if (m_chatPanel) {
        m_chatPanel->setStatusText(text);
    }
}

void MainWindow::openSettingsDialog() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(m_controller, this);
    }
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindow::openProfileDialog() {
    if (!m_controller) {
        return;
    }
    if (!m_profileDialog) {
        m_profileDialog = new ProfileDialog(m_controller, this);
    }
    m_profileDialog->show();
    m_profileDialog->raise();
    m_profileDialog->activateWindow();
}

void MainWindow::handleFileReceived(const PeerInfo &peer, const QString &roleName, const QString &fileName,
                                    const QString &path) {
    if (!m_chatPanel) {
        return;
    }
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString sender = roleName.isEmpty() ? peer.displayName : QStringLiteral("%1(%2)").arg(peer.displayName, roleName);
    const QString message =
        LanguageManager::text(LangKey::MainWindow::FileSaved, QStringLiteral("发送的文件 %1 已保存到 %2"))
            .arg(fileName, path);
    m_chatPanel->appendTimelineHint(
        timestamp, LanguageManager::text(LangKey::MainWindow::FileTag, QStringLiteral("文件")));
    m_chatPanel->appendIncomingMessage(timestamp, sender, message);
}

void MainWindow::handleShareCatalog(const QString &peerId, const QList<SharedFileInfo> &files) {
    m_remoteShares.insert(peerId, files);
    if (m_shareDialog && m_shareDialog->isVisible() && peerId == m_currentPeerId) {
        m_shareDialog->setRemoteEntries(files);
    }
}

void MainWindow::openShareCenter() {
    if (!m_controller) {
        return;
    }
    if (!m_shareDialog) {
        m_shareDialog = new ShareCenterDialog(m_controller, this);
    }
    m_shareDialog->setPeerId(m_currentPeerId);
    m_shareDialog->setRemoteEntries(m_remoteShares.value(m_currentPeerId));
    m_shareDialog->show();
    m_shareDialog->raise();
    m_shareDialog->activateWindow();
}
