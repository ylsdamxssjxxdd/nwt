#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAbstractSocket>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QPainter>
#include <QStringList>
#include <QStyle>
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
    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::handleSend);
    connect(m_controller, &ChatController::chatMessageReceived, this, &MainWindow::appendMessage);
    connect(m_controller, &ChatController::statusInfo, this, &MainWindow::showStatus);
    connect(m_controller, &ChatController::controllerWarning, this, &MainWindow::showStatus);
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
}

QWidget *MainWindow::buildLeftPanel(QWidget *parent) {
    auto *panel = new QFrame(parent);
    panel->setObjectName("contactPanel");
    panel->setMinimumWidth(320);
    panel->setMaximumWidth(380);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(buildTopBanner(panel));
    layout->addWidget(buildProfileCard(panel));
    layout->addWidget(buildSearchRow(panel));
    layout->addWidget(buildTabBar(panel));
    layout->addWidget(buildPeerSection(panel), 1);
    layout->addWidget(buildBottomToolbar(panel));

    applyContactPanelStyle(panel);
    return panel;
}

QWidget *MainWindow::buildChatPanel(QWidget *parent) {
    auto *panel = new QFrame(parent);
    panel->setObjectName("chatPanel");

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *titleRow = new QHBoxLayout();
    auto *title = new QLabel(tr("对话区"), panel);
    title->setObjectName("chatTitle");
    titleRow->addWidget(title);
    titleRow->addStretch();

    m_addSubnetButton = new QPushButton(tr("添加网段"), panel);
    m_addSubnetButton->setObjectName("actionButton");
    titleRow->addWidget(m_addSubnetButton);
    layout->addLayout(titleRow);

    m_chatLog = new QTextEdit(panel);
    m_chatLog->setObjectName("chatLog");
    m_chatLog->setReadOnly(true);
    m_chatLog->setPlaceholderText(tr("选择联系人后即可开始对话…"));
    layout->addWidget(m_chatLog, 1);

    auto *inputCard = new QFrame(panel);
    inputCard->setObjectName("inputCard");
    auto *inputLayout = new QHBoxLayout(inputCard);
    inputLayout->setContentsMargins(12, 8, 12, 8);
    inputLayout->setSpacing(8);

    m_input = new QLineEdit(inputCard);
    m_input->setPlaceholderText(tr("输入消息内容，按回车发送"));
    inputLayout->addWidget(m_input, 1);

    m_sendButton = new QPushButton(tr("发送"), inputCard);
    m_sendButton->setObjectName("sendButton");
    inputLayout->addWidget(m_sendButton);
    layout->addWidget(inputCard);

    m_statusLabel = new QLabel(tr("未连接"), panel);
    m_statusLabel->setObjectName("statusLabel");
    layout->addWidget(m_statusLabel);

    panel->setStyleSheet(R"(
        #chatPanel {
            background: #ffffff;
        }
        #chatTitle {
            font-size: 18px;
            color: #414d63;
            font-weight: 600;
        }
        #chatLog {
            border: 1px solid #d2e0f0;
            border-radius: 12px;
            background: #f7f9fd;
            padding: 12px;
            font-size: 14px;
        }
        #inputCard {
            border: 1px solid #e0e0e0;
            border-radius: 12px;
            background: #fffefa;
        }
        #sendButton {
            background: #1e8bcb;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 8px 18px;
            font-weight: 600;
        }
        #sendButton:hover {
            background: #1674ac;
        }
        #statusLabel {
            color: #888888;
            font-size: 12px;
        }
        #actionButton {
            border: 1px solid #f4b552;
            border-radius: 18px;
            padding: 6px 16px;
            background: #fff4d5;
            color: #c87900;
        }
        #actionButton:hover {
            background: #ffefd1;
        }
    )");

    return panel;
}

QWidget *MainWindow::buildTopBanner(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("topBanner");
    frame->setFixedHeight(48);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(8);

    auto *unitLabel = new QLabel(tr("单位名称"), frame);
    unitLabel->setObjectName("unitLabel");
    layout->addWidget(unitLabel);

    auto *feedbackButton = new QPushButton(tr("反馈"), frame);
    feedbackButton->setObjectName("feedbackButton");
    feedbackButton->setFlat(true);
    layout->addWidget(feedbackButton);

    layout->addStretch();

    auto *themeButton = new QToolButton(frame);
    themeButton->setObjectName("toolbarIcon");
    themeButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    layout->addWidget(themeButton);

    auto *minButton = new QToolButton(frame);
    minButton->setObjectName("toolbarIcon");
    minButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    layout->addWidget(minButton);

    auto *closeButton = new QToolButton(frame);
    closeButton->setObjectName("toolbarIcon");
    closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    layout->addWidget(closeButton);

    connect(feedbackButton, &QPushButton::clicked, this,
            [this]() { showStatus(tr("反馈已记录，我们会尽快答复。")); });
    connect(themeButton, &QToolButton::clicked, this,
            [this]() { showStatus(tr("主题商店功能开发中，敬请期待。")); });
    connect(minButton, &QToolButton::clicked, this, &MainWindow::showMinimized);
    connect(closeButton, &QToolButton::clicked, this, &MainWindow::close);

    return frame;
}

QWidget *MainWindow::buildProfileCard(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("profileCard");
    frame->setFixedHeight(138);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    m_avatarLabel = new QLabel(tr("E"), frame);
    m_avatarLabel->setObjectName("avatarLabel");
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setMinimumSize(70, 70);
    layout->addWidget(m_avatarLabel);

    auto *infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(4);

    auto *nameRow = new QHBoxLayout();
    nameRow->setSpacing(6);
    m_profileName = new QLabel(tr("EVA-0"), frame);
    m_profileName->setObjectName("profileName");
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

    m_profileSignature = new QLabel(tr("编辑个性签名"), frame);
    m_profileSignature->setObjectName("profileSignature");
    infoLayout->addWidget(m_profileSignature);

    auto *quickRow = new QHBoxLayout();
    quickRow->setSpacing(8);
    const QStringList quickTags = {tr("淘"), tr("目"), tr("邮"), tr("+")};
    for (const QString &tag : quickTags) {
        auto *label = new QLabel(tag, frame);
        label->setObjectName("quickBadge");
        quickRow->addWidget(label);
    }
    quickRow->addStretch();
    infoLayout->addLayout(quickRow);

    layout->addLayout(infoLayout, 1);

    connect(statusButton, &QToolButton::clicked, this,
            [this]() { showStatus(tr("当前为在线状态，可在此切换。")); });

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

    auto *addButton = new QToolButton(frame);
    addButton->setObjectName("addButton");
    addButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    layout->addWidget(addButton);

    connect(addButton, &QToolButton::clicked, this,
            [this]() { showStatus(tr("统一添加入口正在建设，敬请期待。")); });

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

QWidget *MainWindow::buildBottomToolbar(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("bottomToolbar");
    frame->setFixedHeight(56);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(16);

    const QVector<QStyle::StandardPixmap> buttons = {
        QStyle::SP_TitleBarMenuButton,
        QStyle::SP_MediaVolume,
        QStyle::SP_FileDialogInfoView,
        QStyle::SP_FileDialogContentsView,
        QStyle::SP_DialogOpenButton,
        QStyle::SP_BrowserReload
    };

    for (int i = 0; i < buttons.size(); ++i) {
        auto *tool = new QToolButton(frame);
        tool->setObjectName("bottomButton");
        tool->setIcon(style()->standardIcon(buttons[i]));
        tool->setToolButtonStyle(Qt::ToolButtonIconOnly);
        layout->addWidget(tool);

        connect(tool, &QToolButton::clicked, this, [this, i]() {
            switch (i) {
            case 0:
                showStatus(tr("功能菜单准备中。"));
                break;
            case 1:
                showStatus(tr("语音通知暂未开启。"));
                break;
            default:
                showStatus(tr("该功能正在迭代更新。"));
                break;
            }
        });
    }

    layout->addStretch();
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

    panel->setStyleSheet(R"(
        #contactPanel {
            background: #fff6dc;
            border-right: 1px solid #e3c779;
            color: #5a4e3b;
        }
        #topBanner {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #f4d26f, stop:1 #e79f1a);
            color: #ffffff;
        }
        #unitLabel {
            font-size: 16px;
            font-weight: 600;
        }
        QPushButton#feedbackButton {
            color: #ffffff;
            font-size: 13px;
        }
        QToolButton#toolbarIcon {
            border: none;
            color: #ffffff;
        }
        #profileCard {
            background: #fbeab6;
            border-bottom: 1px solid #f4d98e;
        }
        #avatarLabel {
            background: #4a90e2;
            color: #ffffff;
            border-radius: 10px;
            font-size: 26px;
            font-weight: 600;
        }
        #profileName {
            font-size: 18px;
            color: #5a4e3b;
            font-weight: 600;
        }
        #profileSignature {
            color: #836c3e;
            font-size: 13px;
        }
        #statusDot {
            background: #3bc15b;
            border-radius: 6px;
        }
        QToolButton#statusDropdown {
            border: none;
            color: #b77a13;
        }
        #quickBadge {
            background: #f8cf7a;
            color: #a15f00;
            border-radius: 12px;
            padding: 4px 10px;
            font-weight: 600;
        }
        #searchRow {
            background: #fdf1c8;
        }
        #searchField {
            border: 1px solid #f1c966;
            border-radius: 18px;
            padding: 6px 12px;
            background: rgba(255, 255, 255, 0.9);
        }
        QToolButton#addButton {
            border: none;
            background: transparent;
            color: #c07b00;
        }
        #tabBar {
            background: #f0d796;
            border-top: 1px solid #f9e4b6;
            border-bottom: 1px solid #f9e4b6;
        }
        QToolButton#tabButton {
            border: none;
            padding: 6px;
            color: #c0a066;
        }
        QToolButton#tabButton:hover {
            color: #fdfdfd;
        }
        QToolButton#tabButton:checked {
            background: #fdf6e5;
            border-radius: 12px;
            color: #1c8fc8;
        }
        #peerSection {
            background: #fffaf1;
        }
        #peerList {
            background: transparent;
            border: none;
        }
        #emptyIcon {
            background: #ded6c5;
            border-radius: 12px;
            font-size: 20px;
            color: #7c6d58;
        }
        #emptyText {
            color: #9c8665;
        }
        #bottomToolbar {
            background: #f0d796;
            border-top: 1px solid #e4c680;
        }
        QToolButton#bottomButton {
            border: none;
            color: #a47d45;
        }
    )");
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
}

void MainWindow::handleSend() {
    if (!m_controller) {
        return;
    }

    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    if (m_currentPeerId.isEmpty()) {
        showStatus(tr("请先选择一个联系人"));
        return;
    }

    m_controller->sendMessageToPeer(m_currentPeerId, text);
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_chatLog->append(QStringLiteral("[%1] 我: %2").arg(timestamp, text));
    m_input->clear();
}

void MainWindow::handlePeerSelection(const QModelIndex &index) {
    if (!index.isValid()) {
        return;
    }

    m_currentPeerId = index.data(PeerDirectory::IdRole).toString();
    const QString display = index.data(Qt::DisplayRole).toString();
    showStatus(tr("已选择 %1").arg(display));
}

void MainWindow::appendMessage(const PeerInfo &peer, const QString &text) {
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString label = peer.displayName.isEmpty() ? peer.id : peer.displayName;
    m_chatLog->append(QStringLiteral("[%1] %2: %3").arg(timestamp, label, text));
}

void MainWindow::showStatus(const QString &text) {
    if (m_statusLabel) {
        m_statusLabel->setText(text);
    }
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
