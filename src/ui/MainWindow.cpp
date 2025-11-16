#include "MainWindow.h"
#include "ProfileDialog.h"
#include "SettingsDialog.h"
#include "ShareCenterDialog.h"
#include "StyleHelper.h"

#include <QAbstractItemView>
#include <QAbstractSocket>
#include <QEvent>
#include <QFileDialog>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStringList>
#include <QStyle>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
int defaultPrefix(const QHostAddress &address) {
    return address.protocol() == QAbstractSocket::IPv6Protocol ? 64 : 24;
}

QHostAddress normalizeNetwork(const QHostAddress &address, int prefixLength) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol && prefixLength >= 0 && prefixLength <= 32) {
        const quint32 ip = address.toIPv4Address();
        const quint32 mask = prefixLength == 0 ? 0 : 0xFFFFFFFFu << (32 - prefixLength);
        return QHostAddress(ip & mask);
    }
    return address;
}
} // namespace

MainWindow::MainWindow(ChatController *controller, QWidget *parent)
    : QMainWindow(parent), m_controller(controller) {
    setupUi();
    if (!m_controller) {
        return;
    }

    if (auto *directory = m_controller->peerDirectory()) {
        m_peerList->setModel(directory);
        connect(directory, &PeerDirectory::peerListChanged, this, &MainWindow::updatePeerPlaceholder);
    }

    if (auto *selection = m_peerList->selectionModel()) {
        connect(selection, &QItemSelectionModel::currentChanged, this,
                [this](const QModelIndex &current, const QModelIndex &) { handlePeerSelection(current); });
    }
    connect(m_peerList, &QListView::clicked, this, &MainWindow::handlePeerSelection);

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::handleSend);
    connect(m_fileButton, &QPushButton::clicked, this, &MainWindow::handleSendFile);
    connect(m_shareButton, &QPushButton::clicked, this, &MainWindow::openShareCenter);
    connect(m_controller, &ChatController::chatMessageReceived, this, &MainWindow::appendMessage);
    connect(m_controller, &ChatController::fileReceived, this, &MainWindow::handleFileReceived);
    connect(m_controller, &ChatController::shareCatalogReceived, this, &MainWindow::handleShareCatalog);
    connect(m_controller, &ChatController::statusInfo, this, &MainWindow::showStatus);
    connect(m_controller, &ChatController::controllerWarning, this, &MainWindow::showStatus);
    connect(m_controller, &ChatController::roleChanged, this, [this]() { refreshProfileCard(); });
    connect(m_controller, &ChatController::profileUpdated, this, [this](const ProfileDetails &) { refreshProfileCard(); });
    connect(m_addSubnetButton, &QPushButton::clicked, this, &MainWindow::handleAddSubnet);

    updatePeerPlaceholder();
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_contactPanel = buildLeftPanel(central);
    rootLayout->addWidget(m_contactPanel, 0);

    auto *chatPanel = buildChatPanel(central);
    rootLayout->addWidget(chatPanel, 1);

    setCentralWidget(central);
    setMinimumSize(960, 600);
    setWindowTitle(tr("内网通 - 轻松联络每一位同事"));
    refreshProfileCard();
}

QWidget *MainWindow::buildLeftPanel(QWidget *parent) {
    auto *panel = new QFrame(parent);
    panel->setObjectName("contactPanel");
    panel->setMinimumWidth(320);
    panel->setMaximumWidth(380);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(buildProfileCard(panel));
    layout->addWidget(buildSearchRow(panel));
    layout->addWidget(buildTabBar(panel));
    layout->addWidget(buildPeerSection(panel), 1);

    applyContactPanelStyle(panel);
    return panel;
}

QWidget *MainWindow::buildChatPanel(QWidget *parent) {
    auto *panel = new QFrame(parent);
    panel->setObjectName("chatPanel");

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QFrame(panel);
    header->setObjectName("chatHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 18, 24, 18);
    headerLayout->setSpacing(16);

    auto *headerInfo = new QVBoxLayout();
    headerInfo->setContentsMargins(0, 0, 0, 0);
    headerInfo->setSpacing(4);
    m_chatNameLabel = new QLabel(tr("选择联系人"), header);
    m_chatNameLabel->setObjectName("chatName");
    headerInfo->addWidget(m_chatNameLabel);

    m_chatPresenceLabel = new QLabel(tr("尚未建立连接"), header);
    m_chatPresenceLabel->setObjectName("chatPresence");
    headerInfo->addWidget(m_chatPresenceLabel);
    headerLayout->addLayout(headerInfo);
    headerLayout->addStretch();

    m_addSubnetButton = new QPushButton(tr("添加子网"), header);
    m_addSubnetButton->setObjectName("actionButton");
    headerLayout->addWidget(m_addSubnetButton);
    layout->addWidget(header);

    auto *body = new QFrame(panel);
    body->setObjectName("chatBody");
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_messageScroll = new QScrollArea(body);
    m_messageScroll->setObjectName("messageScroll");
    m_messageScroll->setWidgetResizable(true);
    m_messageScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_messageViewport = new QWidget(m_messageScroll);
    m_messageViewport->setObjectName("messageViewport");
    m_messageLayout = new QVBoxLayout(m_messageViewport);
    m_messageLayout->setContentsMargins(32, 24, 32, 24);
    m_messageLayout->setSpacing(12);

    m_chatEmptyLabel = new QLabel(tr("选择联系人后即可开始对话…"), m_messageViewport);
    m_chatEmptyLabel->setObjectName("chatEmpty");
    m_chatEmptyLabel->setAlignment(Qt::AlignCenter);
    m_chatEmptyLabel->setWordWrap(true);
    m_messageLayout->addWidget(m_chatEmptyLabel, 0, Qt::AlignCenter);
    m_messageLayout->addStretch(1);

    m_messageScroll->setWidget(m_messageViewport);
    bodyLayout->addWidget(m_messageScroll);
    layout->addWidget(body, 1);

    auto *inputArea = new QFrame(panel);
    inputArea->setObjectName("chatInputArea");
    auto *inputLayout = new QVBoxLayout(inputArea);
    inputLayout->setContentsMargins(32, 16, 32, 16);
    inputLayout->setSpacing(12);

    auto *toolRow = new QHBoxLayout();
    toolRow->setContentsMargins(0, 0, 0, 0);
    toolRow->setSpacing(12);
    auto buildToolButton = [inputArea](const QString &text) {
        auto *btn = new QToolButton(inputArea);
        btn->setObjectName("chatTool");
        btn->setAutoRaise(true);
        btn->setText(text);
        return btn;
    };
    toolRow->addWidget(buildToolButton(tr("表情")));
    toolRow->addWidget(buildToolButton(tr("收藏")));
    toolRow->addWidget(buildToolButton(tr("截屏")));

    m_fileButton = new QPushButton(tr("文件"), inputArea);
    m_fileButton->setObjectName("chatToolButton");
    m_fileButton->setFlat(true);
    toolRow->addWidget(m_fileButton);

    m_shareButton = new QPushButton(tr("共享"), inputArea);
    m_shareButton->setObjectName("chatToolButton");
    m_shareButton->setFlat(true);
    toolRow->addWidget(m_shareButton);
    toolRow->addStretch(1);
    inputLayout->addLayout(toolRow);

    m_inputEdit = new QTextEdit(inputArea);
    m_inputEdit->setObjectName("chatInput");
    m_inputEdit->setPlaceholderText(tr("Enter 发送，Shift+Enter 换行"));
    m_inputEdit->setMinimumHeight(110);
    m_inputEdit->setMaximumHeight(150);
    m_inputEdit->installEventFilter(this);
    inputLayout->addWidget(m_inputEdit);

    auto *sendRow = new QHBoxLayout();
    sendRow->setContentsMargins(0, 0, 0, 0);
    sendRow->setSpacing(12);
    sendRow->addStretch(1);
    m_sendButton = new QPushButton(tr("发 送 (S)"), inputArea);
    m_sendButton->setObjectName("sendButton");
    sendRow->addWidget(m_sendButton);
    inputLayout->addLayout(sendRow);
    layout->addWidget(inputArea, 0);

    auto *statusBar = new QFrame(panel);
    statusBar->setObjectName("chatStatusBar");
    auto *statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(32, 8, 32, 8);
    statusLayout->setSpacing(8);
    m_statusLabel = new QLabel(tr("未连接"), statusBar);
    m_statusLabel->setObjectName("statusLabel");
    statusLayout->addWidget(m_statusLabel);
    layout->addWidget(statusBar, 0);

    panel->setStyleSheet(UiUtils::loadStyleSheet(QStringLiteral(":/ui/qss/ChatPanel.qss")));

    return panel;
}QWidget *MainWindow::buildProfileCard(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("profileCard");
    frame->setFixedHeight(138);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    m_avatarLabel = new QPushButton(tr("E"), frame);
    m_avatarLabel->setObjectName("avatarLabel");
    m_avatarLabel->setFixedSize(88, 88);
    m_avatarLabel->setFlat(true);
    m_avatarLabel->setFocusPolicy(Qt::NoFocus);
    m_avatarLabel->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_avatarLabel);
    layout->setAlignment(m_avatarLabel, Qt::AlignTop);

    auto *infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(4);

    auto *nameRow = new QHBoxLayout();
    nameRow->setSpacing(6);
    const ProfileDetails profile = m_controller ? m_controller->profileDetails() : ProfileDetails{};
    const QString defaultName =
        profile.name.isEmpty() ? (m_controller ? m_controller->localDisplayName() : tr("EVA-0")) : profile.name;
    m_profileName = new QLabel(defaultName, frame);
    m_profileName->setObjectName("profileName");
    m_profileName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameRow->addWidget(m_profileName);

    auto *statusDot = new QLabel(frame);
    statusDot->setObjectName("statusDot");
    statusDot->setFixedSize(12, 12);
    nameRow->addWidget(statusDot);

    auto *statusButton = new QToolButton(frame);
    statusButton->setObjectName("statusDropdown");
    statusButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    nameRow->addWidget(statusButton);
    nameRow->addStretch();
    infoLayout->addLayout(nameRow);

    const QString signatureText = m_controller ? m_controller->signatureText() : tr("编辑个性签名");
    m_profileSignature = new QLabel(signatureText, frame);
    m_profileSignature->setObjectName("profileSignature");
    m_profileSignature->setWordWrap(true);
    m_profileSignature->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    infoLayout->addWidget(m_profileSignature);

    layout->addLayout(infoLayout, 1);

    auto createProfileAction = [frame](QStyle::StandardPixmap icon, const QString &tooltip) {
        auto *button = new QToolButton(frame);
        button->setObjectName("profileAction");
        button->setIcon(frame->style()->standardIcon(icon));
        button->setToolTip(tooltip);
        button->setAutoRaise(true);
        return button;
    };
    auto *actionLayout = new QVBoxLayout();
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    auto *refreshButton = createProfileAction(QStyle::SP_BrowserReload, tr("刷新状态"));
    auto *settingsButton = createProfileAction(QStyle::SP_FileDialogDetailedView, tr("设置中心"));
    actionLayout->addWidget(refreshButton, 0, Qt::AlignRight);
    actionLayout->addWidget(settingsButton, 0, Qt::AlignRight);
    actionLayout->addStretch(1);
    layout->addLayout(actionLayout);

    connect(statusButton, &QToolButton::clicked, this,
            [this]() { showStatus(tr("当前为在线状态，可在此切换。")); });
    connect(m_avatarLabel, &QPushButton::clicked, this, &MainWindow::openProfileDialog);
    connect(refreshButton, &QToolButton::clicked, this,
            [this]() { showStatus(tr("正在刷新状态...")); });
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::openSettingsDialog);

    return frame;
}

QWidget *MainWindow::buildSearchRow(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("searchRow");
    frame->setFixedHeight(64);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);

    m_searchEdit = new QLineEdit(frame);
    m_searchEdit->setObjectName("searchField");
    m_searchEdit->setPlaceholderText(tr("搜索联系人、群或应用"));
    layout->addWidget(m_searchEdit, 1);

    return frame;
}

QWidget *MainWindow::buildTabBar(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("tabBar");
    frame->setFixedHeight(60);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(20, 4, 20, 8);
    layout->setSpacing(12);

    struct TabConfig {
        QStyle::StandardPixmap icon;
        QString tooltip;
    };

    const QVector<TabConfig> configs = {
        {QStyle::SP_MessageBoxInformation, tr("最近联系人")},
        {QStyle::SP_FileDialogDetailedView, tr("所有联系人")},
        {QStyle::SP_ComputerIcon, tr("组织架构")},
        {QStyle::SP_FileDialogContentsView, tr("公共服务")}
    };

    m_tabButtons.clear();
    for (int i = 0; i < configs.size(); ++i) {
        auto *button = new QToolButton(frame);
        button->setObjectName("tabButton");
        button->setIcon(style()->standardIcon(configs[i].icon));
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setToolTip(configs[i].tooltip);
        layout->addWidget(button);
        m_tabButtons.append(button);

        connect(button, &QToolButton::clicked, this, [this, i]() { setActiveTab(i); });
    }

    layout->addStretch();
    setActiveTab(0);

    return frame;
}

QWidget *MainWindow::buildPeerSection(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("peerSection");

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 0, 16, 12);
    layout->setSpacing(8);

    m_peerStack = new QStackedWidget(frame);
    m_peerStack->setObjectName("peerStack");

    m_peerList = new QListView(frame);
    m_peerList->setObjectName("peerList");
    m_peerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_peerList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_peerList->setSpacing(6);
    m_peerList->setUniformItemSizes(true);
    m_peerStack->addWidget(m_peerList);

    m_emptyState = createEmptyState(frame);
    m_peerStack->addWidget(m_emptyState);
    m_peerStack->setCurrentWidget(m_emptyState);

    layout->addWidget(m_peerStack);
    return frame;
}


QWidget *MainWindow::createEmptyState(QWidget *parent) {
    auto *holder = new QWidget(parent);
    holder->setObjectName("emptyHolder");

    auto *layout = new QVBoxLayout(holder);
    layout->setContentsMargins(0, 60, 0, 60);
    layout->setSpacing(12);

    auto *icon = new QLabel(tr("通"), holder);
    icon->setObjectName("emptyIcon");
    icon->setFixedSize(68, 68);
    icon->setAlignment(Qt::AlignCenter);
    layout->addStretch();
    layout->addWidget(icon, 0, Qt::AlignHCenter);

    auto *text = new QLabel(tr("当前没有最近联系人"), holder);
    text->setObjectName("emptyText");
    text->setAlignment(Qt::AlignHCenter);
    layout->addWidget(text, 0, Qt::AlignHCenter);
    layout->addStretch();

    return holder;
}

void MainWindow::applyContactPanelStyle(QWidget *panel) {
    if (!panel) {
        return;
    }

    panel->setStyleSheet(UiUtils::loadStyleSheet(QStringLiteral(":/ui/qss/ContactPanel.qss")));
}

void MainWindow::setActiveTab(int index) {
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        if (auto *button = m_tabButtons.at(i)) {
            button->setChecked(i == index);
        }
    }
    showStatus(tr("切换到%1").arg(index == 0 ? tr("最近联系人") : tr("其他分栏")));
}

void MainWindow::updatePeerPlaceholder() {
    if (!m_peerStack) {
        return;
    }

    const bool hasPeers = m_controller && m_controller->peerDirectory()
                          && m_controller->peerDirectory()->rowCount() > 0;
    m_peerStack->setCurrentWidget(hasPeers ? static_cast<QWidget *>(m_peerList) : m_emptyState);
    if (!hasPeers) {
        updateChatHeader(QString());
    }
}

void MainWindow::refreshProfileCard() {
    if (!m_controller) {
        return;
    }
    const ProfileDetails profile = m_controller->profileDetails();
    const QString displayName =
        profile.name.isEmpty() ? m_controller->localDisplayName() : profile.name;
    if (m_profileName) {
        m_profileName->setText(displayName);
    }
    if (m_profileSignature) {
        const QString signature = profile.signature.isEmpty() ? tr("编辑个性签名") : profile.signature;
        m_profileSignature->setText(signature);
    }
    if (m_avatarLabel) {
        const QString letter = displayName.left(1).toUpper();
        m_avatarLabel->setText(letter);
    }
}

void MainWindow::updateChatHeader(const QString &displayName) {
    if (m_chatNameLabel) {
        m_chatNameLabel->setText(displayName.isEmpty() ? tr("选择联系人") : displayName);
    }
    if (m_chatPresenceLabel) {
        m_chatPresenceLabel->setText(displayName.isEmpty() ? tr("尚未建立连接") : tr("已就绪，可随时开始对话"));
    }
}

void MainWindow::appendTimelineHint(const QString &timestamp, const QString &text) {
    ensureChatAreaForNewEntry();
    if (!m_messageLayout) {
        return;
    }
    auto *hint = new QLabel(m_messageViewport);
    hint->setObjectName("timelineLabel");
    const QString content = text.isEmpty() ? timestamp : QStringLiteral("%1  %2").arg(timestamp, text);
    hint->setText(content);
    hint->setAlignment(Qt::AlignCenter);
    hint->setTextFormat(Qt::PlainText);
    m_messageLayout->addWidget(hint, 0, Qt::AlignCenter);
    m_messageLayout->addStretch(1);
    scrollToLatestMessage();
}

void MainWindow::appendChatBubble(const QString &timestamp, const QString &sender, const QString &text, bool outgoing) {
    ensureChatAreaForNewEntry();
    if (!m_messageLayout) {
        return;
    }

    auto *row = new QWidget(m_messageViewport);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    auto *avatar = new QLabel(row);
    avatar->setObjectName(outgoing ? "avatarSelf" : "avatarPeer");
    avatar->setFixedSize(40, 40);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setTextFormat(Qt::PlainText);
    avatar->setText(sender.left(1).toUpper());

    auto *bubble = new QFrame(row);
    bubble->setObjectName(outgoing ? "bubbleSelf" : "bubblePeer");
    auto *bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(16, 12, 16, 12);
    bubbleLayout->setSpacing(4);

    if (!outgoing) {
        auto *senderLabel = new QLabel(sender, bubble);
        senderLabel->setObjectName("bubbleSender");
        senderLabel->setTextFormat(Qt::PlainText);
        bubbleLayout->addWidget(senderLabel);
    }

    auto *textLabel = new QLabel(text, bubble);
    textLabel->setObjectName("bubbleText");
    textLabel->setWordWrap(true);
    textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    textLabel->setTextFormat(Qt::PlainText);
    bubbleLayout->addWidget(textLabel);

    auto *timeLabel = new QLabel(timestamp, bubble);
    timeLabel->setObjectName("bubbleSender");
    timeLabel->setAlignment(Qt::AlignRight);
    timeLabel->setTextFormat(Qt::PlainText);
    bubbleLayout->addWidget(timeLabel, 0, Qt::AlignRight);

    if (outgoing) {
        rowLayout->addStretch(1);
        rowLayout->addWidget(bubble);
        rowLayout->addWidget(avatar);
    } else {
        rowLayout->addWidget(avatar);
        rowLayout->addWidget(bubble);
        rowLayout->addStretch(1);
    }

    m_messageLayout->addWidget(row);
    m_messageLayout->addStretch(1);
    scrollToLatestMessage();
}

void MainWindow::ensureChatAreaForNewEntry() {
    if (m_chatEmptyLabel) {
        m_chatEmptyLabel->setVisible(false);
    }
    if (!m_messageLayout || m_messageLayout->count() == 0) {
        return;
    }
    const int lastIndex = m_messageLayout->count() - 1;
    if (auto *lastItem = m_messageLayout->itemAt(lastIndex); lastItem && lastItem->spacerItem()) {
        delete m_messageLayout->takeAt(lastIndex);
    }
}

void MainWindow::scrollToLatestMessage() const {
    if (!m_messageScroll) {
        return;
    }
    if (auto *bar = m_messageScroll->verticalScrollBar()) {
        bar->setValue(bar->maximum());
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_inputEdit && event && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers().testFlag(Qt::ShiftModifier)) {
                return QMainWindow::eventFilter(watched, event);
            }
            handleSend();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::handleSend() {
    if (!m_controller) {
        return;
    }

    const QString text = m_inputEdit ? m_inputEdit->toPlainText().trimmed() : QString();
    if (text.isEmpty()) {
        return;
    }
    if (m_currentPeerId.isEmpty()) {
        showStatus(tr("请先选择一个联系人"));
        return;
    }

    m_controller->sendMessageToPeer(m_currentPeerId, text);
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const ProfileDetails profile = m_controller->profileDetails();
    const QString roleDisplay = profile.name.isEmpty() ? tr("我") : profile.name;
    appendTimelineHint(timestamp, QString());
    appendChatBubble(timestamp, roleDisplay, text, true);
    if (m_inputEdit) {
        m_inputEdit->clear();
        m_inputEdit->setFocus();
    }
}

void MainWindow::handleSendFile() {
    if (!m_controller) {
        return;
    }
    if (m_currentPeerId.isEmpty()) {
        showStatus(tr("请先选择一个联系人"));
        return;
    }
    const QString filePath = QFileDialog::getOpenFileName(this, tr("选择要发送的文件"));
    if (filePath.isEmpty()) {
        return;
    }
    m_controller->sendFileToPeer(m_currentPeerId, filePath);
}

void MainWindow::handlePeerSelection(const QModelIndex &index) {
    if (!index.isValid()) {
        return;
    }

    m_currentPeerId = index.data(PeerDirectory::IdRole).toString();
    const QString display = index.data(Qt::DisplayRole).toString();
    showStatus(tr("已选择 %1").arg(display));
    updateChatHeader(display);
    if (m_shareDialog) {
        m_shareDialog->setPeerId(m_currentPeerId);
        m_shareDialog->setRemoteEntries(m_remoteShares.value(m_currentPeerId));
    }
}

void MainWindow::appendMessage(const PeerInfo &peer, const QString &roleName, const QString &text) {
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString label = peer.displayName.isEmpty() ? peer.id : peer.displayName;
    const QString speaker = roleName.isEmpty() ? label : QStringLiteral("%1(%2)").arg(label, roleName);
    appendTimelineHint(timestamp, QString());
    appendChatBubble(timestamp, speaker, text, false);
}

void MainWindow::showStatus(const QString &text) {
    if (m_statusLabel) {
        m_statusLabel->setText(text);
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
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString sender = roleName.isEmpty() ? peer.displayName : QStringLiteral("%1(%2)").arg(peer.displayName, roleName);
    const QString message = tr("发送文件 %1，已保存到 %2").arg(fileName, path);
    appendTimelineHint(timestamp, tr("文件"));
    appendChatBubble(timestamp, sender, message, false);
}

void MainWindow::handleShareCatalog(const QString &peerId, const QList<SharedFileInfo> &files) {
    m_remoteShares.insert(peerId, files);
    if (m_shareDialog && m_shareDialog->isVisible() && peerId == m_currentPeerId) {
        m_shareDialog->setRemoteEntries(files);
    }
    const QString peerName = peerId == m_currentPeerId ? tr("当前联系人") : peerId;
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString description = tr("收到 %1 的共享目录，共 %2 个文件。").arg(peerName).arg(files.size());
    appendTimelineHint(timestamp, tr("共享"));
    appendChatBubble(timestamp, peerName, description, false);
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

void MainWindow::handleAddSubnet() {
    if (!m_controller) {
        return;
    }

    bool ok = false;
    const QString cidr =
        QInputDialog::getText(this, tr("添加网段"), tr("请输入子网（示例 192.168.1.0/24）："), QLineEdit::Normal,
                              QString(), &ok);
    if (!ok) {
        return;
    }

    const QString trimmed = cidr.trimmed();
    if (trimmed.isEmpty()) {
        showStatus(tr("子网不能为空"));
        return;
    }

    const int slashIndex = trimmed.indexOf('/');
    const QString ipText = (slashIndex == -1 ? trimmed : trimmed.left(slashIndex)).trimmed();
    const QString prefixText = slashIndex == -1 ? QString() : trimmed.mid(slashIndex + 1).trimmed();

    if (ipText.isEmpty()) {
        showStatus(tr("格式应类似 192.168.1.0/24"));
        return;
    }

    QHostAddress network(ipText);
    if (network.isNull()) {
        showStatus(tr("无效的地址: %1").arg(ipText));
        return;
    }

    int prefixLength = -1;
    if (prefixText.isEmpty()) {
        prefixLength = defaultPrefix(network);
    } else {
        bool prefixOk = false;
        prefixLength = prefixText.toInt(&prefixOk);
        if (!prefixOk) {
            showStatus(tr("无效的前缀: %1").arg(prefixText));
            return;
        }
    }

    const int maxPrefix = network.protocol() == QAbstractSocket::IPv6Protocol ? 128 : 32;
    if (prefixLength < 0 || prefixLength > maxPrefix) {
        showStatus(tr("前缀范围应为 0-%1").arg(maxPrefix));
        return;
    }

    const QHostAddress normalized = normalizeNetwork(network, prefixLength);
    m_controller->addSubnet(normalized, prefixLength);
    if (slashIndex == -1) {
        showStatus(tr("按照默认掩码添加 %1/%2").arg(normalized.toString()).arg(prefixLength));
    } else {
        showStatus(tr("已添加 %1/%2").arg(normalized.toString()).arg(prefixLength));
    }
}
