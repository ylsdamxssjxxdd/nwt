#include "SettingsDialog.h"
#include "StyleHelper.h"

#include <QAbstractItemView>
#include <QAbstractSocket>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <functional>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QMessageBox>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSet>
#include <QStringList>
#include <algorithm>

namespace {
QVBoxLayout *sectionLayout(QFrame *section) {
    return qobject_cast<QVBoxLayout *>(section->layout());
}

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

QString subnetDisplay(const QPair<QHostAddress, int> &pair) {
    return QStringLiteral("%1/%2").arg(pair.first.toString()).arg(pair.second);
}

constexpr int RoleToken = Qt::UserRole;
constexpr int RoleRemovable = Qt::UserRole + 1;
} // namespace

SettingsDialog::SettingsDialog(ChatController *controller, QWidget *parent)
    : QDialog(parent), m_controller(controller) {
    setupUi();
}

void SettingsDialog::setupUi() {
    setWindowTitle(tr("设置中心"));
    resize(880, 640);
    setModal(false);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_navGroup = new QButtonGroup(this);
    layout->addWidget(buildNavigation(this));
    layout->addWidget(buildContentStack(this), 1);

    connect(m_navGroup, &QButtonGroup::idClicked, this, &SettingsDialog::switchCategory);
    switchCategory(0);

    setStyleSheet(UiUtils::loadStyleSheet(QStringLiteral(":/ui/qss/SettingsDialog.qss")));
}

QWidget *SettingsDialog::buildNavigation(QWidget *parent) {
    auto *panel = new QFrame(parent);
    panel->setObjectName("navPanel");
    panel->setFixedWidth(180);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 24, 16, 24);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("设置中心"), panel);
    title->setObjectName("navTitle");
    layout->addWidget(title);

    struct NavItem {
        QString text;
        QStyle::StandardPixmap icon;
    };
    const QVector<NavItem> items = {
        {tr("常规设置"), QStyle::SP_DialogYesButton},
        {tr("网络设置"), QStyle::SP_DriveNetIcon},
        {tr("通知设置"), QStyle::SP_MediaVolume},
        {tr("热键设置"), QStyle::SP_CommandLink},
        {tr("内网通锁"), QStyle::SP_DialogResetButton},
        {tr("邮件设置"), QStyle::SP_DialogOpenButton}};

    for (int i = 0; i < items.size(); ++i) {
        auto *button = new QToolButton(panel);
        button->setObjectName("navButton");
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setIcon(style()->standardIcon(items[i].icon));
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setText(items[i].text);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_navGroup->addButton(button, i);
        layout->addWidget(button);
        if (i == 0) {
            button->setChecked(true);
        }
    }

    layout->addStretch(1);
    return panel;
}

QWidget *SettingsDialog::buildContentStack(QWidget *parent) {
    auto *panel = new QFrame(parent);
    panel->setObjectName("contentArea");

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(panel);
    m_stack->addWidget(wrapScroll(createGeneralPage()));
    m_stack->addWidget(wrapScroll(createNetworkPage()));
    m_stack->addWidget(wrapScroll(createNotificationPage()));
    m_stack->addWidget(wrapScroll(createHotkeyPage()));
    m_stack->addWidget(wrapScroll(createLockPage()));
    m_stack->addWidget(wrapScroll(createMailPage()));
    layout->addWidget(m_stack);

    return panel;
}

QWidget *SettingsDialog::wrapScroll(QWidget *page) {
    auto *scroll = new QScrollArea();
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return scroll;
}

QFrame *SettingsDialog::createSection(const QString &title) {
    auto *section = new QFrame();
    section->setObjectName("section");
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->setSpacing(8);

    auto *label = new QLabel(title, section);
    label->setObjectName("sectionTitle");
    layout->addWidget(label);

    auto *divider = new QFrame(section);
    divider->setObjectName("divider");
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Plain);
    layout->addWidget(divider);

    return section;
}

QWidget *SettingsDialog::createGeneralPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *common = createSection(tr("常规设置"));
    auto *commonLayout = sectionLayout(common);
    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(12);

    struct ItemDef {
        QString text;
        QString name;
    };
    const QVector<ItemDef> leftColumn = {
        {tr("开机自动启动"), QStringLiteral("general_autoStart")},
        {tr("启用局域网更新"), QStringLiteral("general_lanUpdate")},
        {tr("不接收振屏"), QStringLiteral("general_rejectShake")},
        {tr("不给飞鸽飞秋发推荐"), QStringLiteral("general_rejectFeige")},
        {tr("开启飞鸽飞秋分组"), QStringLiteral("general_enableFeigeGroup")}
    };
    const QVector<ItemDef> rightColumn = {
        {tr("自动最小化到托盘"), QStringLiteral("general_autoMinimize")},
        {tr("隐藏自己IP"), QStringLiteral("general_hideIp")},
        {tr("不接收网段分享"), QStringLiteral("general_rejectSegment")},
        {tr("开启自动分组"), QStringLiteral("general_autoGroup")},
        {tr("开启飞鸽飞秋分组"), QStringLiteral("general_autoSubGroup")}
    };

    for (int i = 0; i < leftColumn.size(); ++i) {
        auto *box = new QCheckBox(leftColumn[i].text, common);
        box->setObjectName(leftColumn[i].name);
        grid->addWidget(box, i, 0);
    }
    for (int i = 0; i < rightColumn.size(); ++i) {
        auto *box = new QCheckBox(rightColumn[i].text, common);
        box->setObjectName(rightColumn[i].name);
        grid->addWidget(box, i, 1);
    }
    commonLayout->addLayout(grid);
    layout->addWidget(common);

    auto *messageSection = createSection(tr("接到新消息时"));
    auto *messageLayout = sectionLayout(messageSection);
    auto *messageRow = new QHBoxLayout();
    auto *autoPopup = new QRadioButton(tr("自动弹出"), messageSection);
    autoPopup->setObjectName(QStringLiteral("general_popup"));
    auto *flashHint = new QRadioButton(tr("闪烁提示"), messageSection);
    flashHint->setObjectName(QStringLiteral("general_flash"));
    messageRow->addWidget(autoPopup);
    messageRow->addWidget(flashHint);
    messageRow->addStretch();
    messageLayout->addLayout(messageRow);
    layout->addWidget(messageSection);

    auto *closeSection = createSection(tr("关闭主面板时"));
    auto *closeLayout = sectionLayout(closeSection);
    auto *closeRow = new QHBoxLayout();
    auto *exitApp = new QRadioButton(tr("退出程序"), closeSection);
    exitApp->setObjectName(QStringLiteral("general_closeExit"));
    auto *minimizeTask = new QRadioButton(tr("最小化到任务栏"), closeSection);
    minimizeTask->setObjectName(QStringLiteral("general_closeTaskbar"));
    closeRow->addWidget(exitApp);
    closeRow->addWidget(minimizeTask);
    closeRow->addStretch();
    closeLayout->addLayout(closeRow);
    layout->addWidget(closeSection);

    auto *friendSection = createSection(tr("好友信息显示"));
    auto *friendLayout = sectionLayout(friendSection);
    auto *friendRow = new QHBoxLayout();
    auto *showSignature = new QRadioButton(tr("显示签名"), friendSection);
    showSignature->setObjectName(QStringLiteral("general_showSignature"));
    auto *showIp = new QRadioButton(tr("显示IP"), friendSection);
    showIp->setObjectName(QStringLiteral("general_showIp"));
    friendRow->addWidget(showSignature);
    friendRow->addWidget(showIp);
    friendRow->addStretch();
    friendLayout->addLayout(friendRow);
    layout->addWidget(friendSection);

    layout->addStretch(1);
    bindGeneralSettings(page);
    return page;
}

QWidget *SettingsDialog::createNetworkPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *searchSection = createSection(tr("搜索设置"));
    auto *searchLayout = sectionLayout(searchSection);
    auto *searchGrid = new QGridLayout();
    searchGrid->setHorizontalSpacing(18);
    searchGrid->setVerticalSpacing(12);
    searchGrid->setColumnStretch(1, 1);
    auto addSearchRow = [searchGrid, searchSection](int row, const QString &labelText, QWidget *editor) {
        auto *label = new QLabel(labelText, searchSection);
        label->setMinimumWidth(80);
        searchGrid->addWidget(label, row, 0);
        searchGrid->addWidget(editor, row, 1);
    };
    auto *searchPort = new QSpinBox(searchSection);
    searchPort->setRange(1, 65535);
    searchPort->setObjectName(QStringLiteral("net_searchPort"));
    addSearchRow(0, tr("端口："), searchPort);

    auto *orgCode = new QLineEdit(searchSection);
    orgCode->setObjectName(QStringLiteral("net_orgCode"));
    orgCode->setPlaceholderText(tr("用于区分组织/部门（可选）"));
    addSearchRow(1, tr("组织号："), orgCode);
    searchLayout->addLayout(searchGrid);

    auto *searchHint = new QLabel(tr("修改搜索设置后需要重新启动客户端才能生效。"), searchSection);
    searchHint->setObjectName(QStringLiteral("hintLabel"));
    searchHint->setWordWrap(true);
    searchLayout->addWidget(searchHint);
    layout->addWidget(searchSection);

    auto *interopSection = createSection(tr("消息互通"));
    auto *interopLayout = sectionLayout(interopSection);
    auto *interopCheck = new QCheckBox(tr("开启与传统客户端互通"), interopSection);
    interopCheck->setObjectName(QStringLiteral("net_enableInterop"));
    interopLayout->addWidget(interopCheck);
    auto *interopRow = new QHBoxLayout();
    auto *interopLabel = new QLabel(tr("互通端口："), interopSection);
    auto *interopPort = new QSpinBox(interopSection);
    interopPort->setRange(1, 65535);
    interopPort->setObjectName(QStringLiteral("net_interopPort"));
    interopRow->addWidget(interopLabel);
    interopRow->addWidget(interopPort, 0);
    interopRow->addStretch();
    interopLayout->addLayout(interopRow);
    layout->addWidget(interopSection);

    auto *bindingSection = createSection(tr("网卡绑定"));
    auto *bindingLayout = sectionLayout(bindingSection);
    auto *bindCheck = new QCheckBox(tr("启用网卡绑定"), bindingSection);
    bindCheck->setObjectName(QStringLiteral("net_bindInterface"));
    bindingLayout->addWidget(bindCheck);
    auto *bindRow = new QHBoxLayout();
    auto *bindLabel = new QLabel(tr("指定网卡："), bindingSection);
    auto *interfaceCombo = new QComboBox(bindingSection);
    interfaceCombo->setObjectName(QStringLiteral("net_interfaceCombo"));
    interfaceCombo->setEnabled(false);
    interfaceCombo->addItem(tr("自动选择"), QString());
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp) || !iface.flags().testFlag(QNetworkInterface::IsRunning)) {
            continue;
        }
        interfaceCombo->addItem(iface.humanReadableName(), iface.name());
    }
    bindRow->addWidget(bindLabel);
    bindRow->addWidget(interfaceCombo, 1);
    bindingLayout->addLayout(bindRow);
    layout->addWidget(bindingSection);

    auto *segmentSection = createSection(tr("网段设置"));
    auto *segmentLayout = sectionLayout(segmentSection);
    auto *segmentIntro = new QLabel(tr("在此添加需要扫描的网段，系统会定期探测这些网段内的联系人。"), segmentSection);
    segmentIntro->setWordWrap(true);
    segmentLayout->addWidget(segmentIntro);

    auto *currentSubnetLabel = new QLabel(segmentSection);
    currentSubnetLabel->setObjectName(QStringLiteral("net_segmentCurrent"));
    currentSubnetLabel->setWordWrap(true);
    currentSubnetLabel->setText(detectedSubnetHint());
    segmentLayout->addWidget(currentSubnetLabel);

    auto *segmentRow = new QHBoxLayout();
    m_subnetInput = new QLineEdit(segmentSection);
    m_subnetInput->setObjectName(QStringLiteral("net_segmentInput"));
    m_subnetInput->setPlaceholderText(tr("示例：192.168.1.0/24 或 192.168.1.1"));
    auto *addButton = new QPushButton(tr("添加"), segmentSection);
    addButton->setObjectName(QStringLiteral("net_segmentAdd"));
    m_removeSubnetButton = new QPushButton(tr("删除"), segmentSection);
    m_removeSubnetButton->setObjectName(QStringLiteral("net_segmentRemove"));
    m_removeSubnetButton->setEnabled(false);
    segmentRow->addWidget(m_subnetInput, 1);
    segmentRow->addWidget(addButton);
    segmentRow->addWidget(m_removeSubnetButton);
    segmentLayout->addLayout(segmentRow);

    m_subnetList = new QListWidget(segmentSection);
    m_subnetList->setObjectName(QStringLiteral("net_segmentList"));
    m_subnetList->setSelectionMode(QAbstractItemView::SingleSelection);
    segmentLayout->addWidget(m_subnetList);

    m_restrictSubnetCheck = new QCheckBox(tr("仅与以上网段保持网络连接"), segmentSection);
    m_restrictSubnetCheck->setObjectName(QStringLiteral("net_segmentRestrict"));
    segmentLayout->addWidget(m_restrictSubnetCheck);

    auto *segmentHint = new QLabel(tr("新增或删除网段后需重新启动客户端方可完全生效。"), segmentSection);
    segmentHint->setObjectName(QStringLiteral("hintLabel"));
    segmentHint->setWordWrap(true);
    segmentLayout->addWidget(segmentHint);
    layout->addWidget(segmentSection);

    connect(addButton, &QPushButton::clicked, this, &SettingsDialog::handleSubnetAdd);
    connect(m_removeSubnetButton, &QPushButton::clicked, this, &SettingsDialog::handleSubnetRemove);
    connect(m_subnetList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current) {
                if (m_removeSubnetButton) {
                    const bool removable = current && current->data(RoleRemovable).toBool();
                    m_removeSubnetButton->setEnabled(removable);
                }
            });

    auto *blockSection = createSection(tr("网段黑名单"));
    auto *blockLayout = sectionLayout(blockSection);
    auto *blockIntro = new QLabel(tr("加入黑名单的网段将不会进行广播和发现，适用于隔离访客或敏感网络。"), blockSection);
    blockIntro->setWordWrap(true);
    blockLayout->addWidget(blockIntro);

    auto *blockRow = new QHBoxLayout();
    m_blockInput = new QLineEdit(blockSection);
    m_blockInput->setObjectName(QStringLiteral("net_blockInput"));
    m_blockInput->setPlaceholderText(tr("示例：10.10.0.0/16 或 10.10.0.1"));
    auto *blockAddButton = new QPushButton(tr("添加"), blockSection);
    blockAddButton->setObjectName(QStringLiteral("net_blockAdd"));
    m_removeBlockButton = new QPushButton(tr("删除"), blockSection);
    m_removeBlockButton->setObjectName(QStringLiteral("net_blockRemove"));
    m_removeBlockButton->setEnabled(false);
    blockRow->addWidget(m_blockInput, 1);
    blockRow->addWidget(blockAddButton);
    blockRow->addWidget(m_removeBlockButton);
    blockLayout->addLayout(blockRow);

    m_blockList = new QListWidget(blockSection);
    m_blockList->setObjectName(QStringLiteral("net_blockList"));
    m_blockList->setSelectionMode(QAbstractItemView::SingleSelection);
    blockLayout->addWidget(m_blockList);

    auto *blockHint = new QLabel(tr("黑名单同样需要重启客户端后彻底生效。"), blockSection);
    blockHint->setObjectName(QStringLiteral("hintLabel"));
    blockHint->setWordWrap(true);
    blockLayout->addWidget(blockHint);
    layout->addWidget(blockSection);

    connect(blockAddButton, &QPushButton::clicked, this, &SettingsDialog::handleBlockedAdd);
    connect(m_removeBlockButton, &QPushButton::clicked, this, &SettingsDialog::handleBlockedRemove);
    connect(m_blockList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (m_removeBlockButton) {
            m_removeBlockButton->setEnabled(row >= 0);
        }
    });

    auto *refreshSection = createSection(tr("自动刷新"));
    auto *refreshLayout = sectionLayout(refreshSection);
    auto *refreshRow = new QHBoxLayout();
    auto *autoRefresh = new QCheckBox(tr("自动刷新联系人"), refreshSection);
    autoRefresh->setObjectName(QStringLiteral("net_autoRefresh"));
    auto *combo = new QComboBox(refreshSection);
    combo->setObjectName(QStringLiteral("net_refreshCombo"));
    combo->addItems({tr("5分钟"), tr("10分钟"), tr("30分钟"), tr("1小时")});
    refreshRow->addWidget(autoRefresh);
    refreshRow->addStretch();
    refreshLayout->addLayout(refreshRow);

    auto *comboRow = new QHBoxLayout();
    auto *label = new QLabel(tr("刷新频率："), refreshSection);
    comboRow->addWidget(label);
    comboRow->addWidget(combo, 0);
    comboRow->addStretch();
    refreshLayout->addLayout(comboRow);

    layout->addWidget(refreshSection);
    layout->addStretch(1);
    bindNetworkSettings(page);
    refreshSubnetList();
    refreshBlockedList();
    return page;
}
QWidget *SettingsDialog::createNotificationPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *notifySection = createSection(tr("哪些要通知？"));
    auto *notifyLayout = sectionLayout(notifySection);
    struct NotifyDef {
        QString text;
        QString name;
    };
    const QVector<NotifyDef> notifications = {
        {tr("当线上联系人上线时，提醒其他联系人"), QStringLiteral("notify_selfOnline")},
        {tr("当其他联系人上线时，显示通知窗口"), QStringLiteral("notify_peerOnline")},
        {tr("收到即时消息/文件时，显示通知窗口"), QStringLiteral("notify_message")},
        {tr("文件传输成功时，显示通知窗口"), QStringLiteral("notify_file")},
        {tr("邮件数量变化时，显示通知窗口"), QStringLiteral("notify_mailChange")}
    };
    for (const auto &def : notifications) {
        auto *box = new QCheckBox(def.text, notifySection);
        box->setObjectName(def.name);
        notifyLayout->addWidget(box);
    }
    layout->addWidget(notifySection);

    auto *soundSection = createSection(tr("提示声音"));
    auto *soundLayout = sectionLayout(soundSection);
    auto *muteRow = new QHBoxLayout();
    auto *disableSound = new QCheckBox(tr("关闭所有提示声音"), soundSection);
    disableSound->setObjectName(QStringLiteral("notify_muteAll"));
    auto *soundLink = new QLabel(tr("声音目录"), soundSection);
    soundLink->setObjectName("linkLabel");
    muteRow->addWidget(disableSound);
    muteRow->addWidget(soundLink);
    muteRow->addStretch();
    soundLayout->addLayout(muteRow);

    auto *groupSound = new QCheckBox(tr("收到每条群消息都播放提示声音"), soundSection);
    groupSound->setObjectName(QStringLiteral("notify_groupSound"));
    soundLayout->addWidget(groupSound);
    layout->addWidget(soundSection);

    auto *previewSection = createSection(tr("热键设置"));
    auto *previewLayout = sectionLayout(previewSection);
    const QVector<QPair<QString, QString>> shortcuts = {
        {tr("显示/隐藏主窗口"), QStringLiteral("Ctrl + I")},
        {tr("截图"), QStringLiteral("Ctrl + Alt + X")},
        {tr("锁定内网通"), QStringLiteral("Ctrl + L")},
        {tr("打开文件存储目录"), QStringLiteral("Ctrl + R")}};
    for (const auto &pair : shortcuts) {
        auto *row = new QHBoxLayout();
        row->addWidget(new QCheckBox(pair.first, previewSection));
        auto *edit = new QLineEdit(pair.second, previewSection);
        edit->setObjectName("shortcutEdit");
        edit->setReadOnly(true);
        row->addWidget(edit, 0);
        row->addStretch();
        previewLayout->addLayout(row);
    }
    layout->addWidget(previewSection);

    layout->addStretch(1);
    bindNotificationSettings(page);
    return page;
}

QWidget *SettingsDialog::createHotkeyPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *hotkeySection = createSection(tr("热键设置"));
    auto *hotkeyLayout = sectionLayout(hotkeySection);
    const QVector<QPair<QString, QString>> shortcuts = {
        {QStringLiteral("hotkey_toggle"), tr("显示/隐藏主窗口")},
        {QStringLiteral("hotkey_capture"), tr("截图")},
        {QStringLiteral("hotkey_lock"), tr("锁定内网通")},
        {QStringLiteral("hotkey_storage"), tr("打开文件存储目录")}};
    const QVector<QString> defaults = {QStringLiteral("Ctrl + I"), QStringLiteral("Ctrl + Alt + X"),
                                       QStringLiteral("Ctrl + L"), QStringLiteral("Ctrl + R")};
    for (int i = 0; i < shortcuts.size(); ++i) {
        auto *row = new QHBoxLayout();
        auto *box = new QCheckBox(shortcuts[i].second, hotkeySection);
        box->setObjectName(shortcuts[i].first + QStringLiteral("_enable"));
        row->addWidget(box);
        auto *edit = new QLineEdit(defaults.value(i), hotkeySection);
        edit->setObjectName(shortcuts[i].first + QStringLiteral("_edit"));
        row->addWidget(edit, 0);
        row->addStretch();
        hotkeyLayout->addLayout(row);
    }
    layout->addWidget(hotkeySection);

    auto *lockSection = createSection(tr("内网通锁"));
    auto *lockLayout = sectionLayout(lockSection);
    auto *desc = new QLabel(tr("设置锁密码，防止他人操作您的内网通。锁定时不影响消息的接收。"), lockSection);
    desc->setWordWrap(true);
    lockLayout->addWidget(desc);
    auto *passwordButton = new QPushButton(tr("设置密码"), lockSection);
    passwordButton->setObjectName(QStringLiteral("security_passwordBtn"));
    lockLayout->addWidget(passwordButton, 0, Qt::AlignLeft);
    layout->addWidget(lockSection);

    auto *idleSection = createSection(tr("鼠标键盘无动作"));
    auto *idleLayout = sectionLayout(idleSection);
    auto *idleRow = new QHBoxLayout();
    auto *spinLabel = new QLabel(tr("分钟后"), idleSection);
    auto *combo = new QComboBox(idleSection);
    combo->setObjectName(QStringLiteral("security_idleMinutes"));
    combo->addItems({tr("5"), tr("10"), tr("15"), tr("30")});
    idleRow->addWidget(combo, 0);
    idleRow->addWidget(spinLabel);
    idleRow->addStretch();
    idleLayout->addLayout(idleRow);

    auto *statusRow = new QHBoxLayout();
    auto *away = new QRadioButton(tr("切换为“离开”"), idleSection);
    away->setObjectName(QStringLiteral("security_idleAway"));
    auto *lock = new QRadioButton(tr("锁定内网通"), idleSection);
    lock->setObjectName(QStringLiteral("security_idleLock"));
    statusRow->addWidget(away);
    statusRow->addWidget(lock);
    statusRow->addStretch();
    idleLayout->addLayout(statusRow);

    layout->addWidget(idleSection);
    layout->addStretch(1);
    bindHotkeySettings(page);
    return page;
}

QWidget *SettingsDialog::createLockPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *lockSection = createSection(tr("内网通锁"));
    auto *lockLayout = sectionLayout(lockSection);
    auto *desc = new QLabel(
        tr("为防止误操作，可设置独立的锁密码。锁定后，需要输入密码才能恢复操作，同时消息仍会静默接收。"), lockSection);
    desc->setWordWrap(true);
    lockLayout->addWidget(desc);

    auto *form = new QGridLayout();
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);

    auto *passLabel = new QLabel(tr("新密码："), lockSection);
    auto *passEdit = new QLineEdit(lockSection);
    passEdit->setObjectName(QStringLiteral("security_pass"));
    passEdit->setEchoMode(QLineEdit::Password);
    auto *confirmLabel = new QLabel(tr("确认密码："), lockSection);
    auto *confirmEdit = new QLineEdit(lockSection);
    confirmEdit->setObjectName(QStringLiteral("security_confirm"));
    confirmEdit->setEchoMode(QLineEdit::Password);
    form->addWidget(passLabel, 0, 0);
    form->addWidget(passEdit, 0, 1);
    form->addWidget(confirmLabel, 1, 0);
    form->addWidget(confirmEdit, 1, 1);
    lockLayout->addLayout(form);

    auto *btnRow = new QHBoxLayout();
    auto *apply = new QPushButton(tr("保存密码"), lockSection);
    apply->setObjectName(QStringLiteral("security_savePass"));
    apply->setToolTip(tr("仅保存在本机，不上传网络"));
    apply->setObjectName("actionBtn");
    btnRow->addWidget(apply, 0, Qt::AlignLeft);
    btnRow->addStretch();
    lockLayout->addLayout(btnRow);
    layout->addWidget(lockSection);

    auto *tip = new QLabel(tr("提示：密码信息仅保存在本机，可在主界面从新建图标处进行快速锁定。"), page);
    tip->setWordWrap(true);
    layout->addWidget(tip);
    layout->addStretch(1);
    bindSecuritySettings(page);
    return page;
}

QWidget *SettingsDialog::createMailPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *mailSection = createSection(tr("邮件账户"));
    auto *mailLayout = sectionLayout(mailSection);
    auto *mailForm = new QGridLayout();
    mailForm->setHorizontalSpacing(12);
    mailForm->setVerticalSpacing(12);
    const QVector<QPair<QString, QString>> fields = {
       {tr("邮箱地址"), QStringLiteral("name@example.com")},
       {tr("IMAP 服务器"), QStringLiteral("imap.example.com")},
       {tr("SMTP 服务器"), QStringLiteral("smtp.example.com")},
       {tr("端口"), QStringLiteral("465")}};
    for (int i = 0; i < fields.size(); ++i) {
        auto *label = new QLabel(fields[i].first + QStringLiteral("："), mailSection);
        auto *edit = new QLineEdit(fields[i].second, mailSection);
        edit->setObjectName(QStringLiteral("mail_field_%1").arg(i));
        mailForm->addWidget(label, i, 0);
        mailForm->addWidget(edit, i, 1);
    }
    mailLayout->addLayout(mailForm);
    layout->addWidget(mailSection);

    auto *notifySection = createSection(tr("邮件通知"));
    auto *notifyLayout = sectionLayout(notifySection);
    auto *sound = new QCheckBox(tr("新邮件到达时播报提示音"), notifySection);
    sound->setObjectName(QStringLiteral("mail_sound"));
    notifyLayout->addWidget(sound);
    auto *popup = new QCheckBox(tr("新邮件到达时显示通知窗口"), notifySection);
    popup->setObjectName(QStringLiteral("mail_popup"));
    notifyLayout->addWidget(popup);
    layout->addWidget(notifySection);

    layout->addStretch(1);
    bindMailSettings(page);
    return page;
}

void SettingsDialog::switchCategory(int index) {
    if (!m_stack) {
        return;
    }
    m_stack->setCurrentIndex(index);
}

void SettingsDialog::bindGeneralSettings(QWidget *section) {
    if (!section || !m_controller) {
        return;
    }
    const GeneralSettings general = m_controller->settings().general;
    auto bindCheck = [this, section](const QString &name, bool value,
                                     std::function<void(GeneralSettings &, bool)> mutate) {
        if (auto *box = section->findChild<QCheckBox *>(name)) {
            box->setChecked(value);
            connect(box, &QCheckBox::toggled, this, [this, mutate](bool state) {
                if (!m_controller) {
                    return;
                }
                auto data = m_controller->settings().general;
                mutate(data, state);
                m_controller->updateGeneralSettings(data);
            });
        }
    };

    bindCheck(QStringLiteral("general_autoStart"), general.autoStart,
              [](GeneralSettings &cfg, bool state) { cfg.autoStart = state; });
    bindCheck(QStringLiteral("general_lanUpdate"), general.enableLanUpdate,
              [](GeneralSettings &cfg, bool state) { cfg.enableLanUpdate = state; });
    bindCheck(QStringLiteral("general_rejectShake"), general.rejectShake,
              [](GeneralSettings &cfg, bool state) { cfg.rejectShake = state; });
    bindCheck(QStringLiteral("general_rejectFeige"), general.rejectFeigeAds,
              [](GeneralSettings &cfg, bool state) { cfg.rejectFeigeAds = state; });
    bindCheck(QStringLiteral("general_enableFeigeGroup"), general.enableFeigeGroup,
              [](GeneralSettings &cfg, bool state) { cfg.enableFeigeGroup = state; });
    bindCheck(QStringLiteral("general_autoMinimize"), general.autoMinimize,
              [](GeneralSettings &cfg, bool state) { cfg.autoMinimize = state; });
    bindCheck(QStringLiteral("general_hideIp"), general.hideIpAddress,
              [](GeneralSettings &cfg, bool state) { cfg.hideIpAddress = state; });
    bindCheck(QStringLiteral("general_rejectSegment"), general.rejectSegmentShare,
              [](GeneralSettings &cfg, bool state) { cfg.rejectSegmentShare = state; });
    bindCheck(QStringLiteral("general_autoGroup"), general.enableAutoGroup,
              [](GeneralSettings &cfg, bool state) { cfg.enableAutoGroup = state; });
    bindCheck(QStringLiteral("general_autoSubGroup"), general.enableAutoSubGroup,
              [](GeneralSettings &cfg, bool state) { cfg.enableAutoSubGroup = state; });

    if (auto *popup = section->findChild<QRadioButton *>(QStringLiteral("general_popup"))) {
        popup->setChecked(general.popupOnMessage);
        connect(popup, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto data = m_controller->settings().general;
            data.popupOnMessage = true;
            data.flashOnMessage = false;
            m_controller->updateGeneralSettings(data);
        });
    }
    if (auto *flash = section->findChild<QRadioButton *>(QStringLiteral("general_flash"))) {
        flash->setChecked(!general.popupOnMessage);
        connect(flash, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto data = m_controller->settings().general;
            data.popupOnMessage = false;
            data.flashOnMessage = true;
            m_controller->updateGeneralSettings(data);
        });
    }

    if (auto *exitRadio = section->findChild<QRadioButton *>(QStringLiteral("general_closeExit"))) {
        exitRadio->setChecked(general.closeBehavior == QStringLiteral("exit"));
        connect(exitRadio, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto data = m_controller->settings().general;
            data.closeBehavior = QStringLiteral("exit");
            m_controller->updateGeneralSettings(data);
        });
    }
    if (auto *taskRadio = section->findChild<QRadioButton *>(QStringLiteral("general_closeTaskbar"))) {
        taskRadio->setChecked(general.closeBehavior != QStringLiteral("exit"));
        connect(taskRadio, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto data = m_controller->settings().general;
            data.closeBehavior = QStringLiteral("taskbar");
            m_controller->updateGeneralSettings(data);
        });
    }

    if (auto *signatureRadio = section->findChild<QRadioButton *>(QStringLiteral("general_showSignature"))) {
        signatureRadio->setChecked(general.friendDisplay == QStringLiteral("signature"));
        connect(signatureRadio, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto data = m_controller->settings().general;
            data.friendDisplay = QStringLiteral("signature");
            m_controller->updateGeneralSettings(data);
        });
    }
    if (auto *ipRadio = section->findChild<QRadioButton *>(QStringLiteral("general_showIp"))) {
        ipRadio->setChecked(general.friendDisplay == QStringLiteral("ip"));
        connect(ipRadio, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto data = m_controller->settings().general;
            data.friendDisplay = QStringLiteral("ip");
            m_controller->updateGeneralSettings(data);
        });
    }
}

void SettingsDialog::bindNetworkSettings(QWidget *section) {
    if (!section || !m_controller) {
        return;
    }
    const NetworkSettings settings = m_controller->settings().network;
    if (auto *portSpin = section->findChild<QSpinBox *>(QStringLiteral("net_searchPort"))) {
        portSpin->setValue(settings.searchPort);
        connect(portSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            prefs.searchPort = static_cast<quint16>(value);
            m_controller->updateNetworkSettings(prefs);
        });
    }
    if (auto *orgEdit = section->findChild<QLineEdit *>(QStringLiteral("net_orgCode"))) {
        orgEdit->setText(settings.organizationCode);
        connect(orgEdit, &QLineEdit::editingFinished, this, [this, orgEdit]() {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            prefs.organizationCode = orgEdit->text().trimmed();
            m_controller->updateNetworkSettings(prefs);
        });
    }
    if (auto *interopCheck = section->findChild<QCheckBox *>(QStringLiteral("net_enableInterop"))) {
        interopCheck->setChecked(settings.enableInterop);
        connect(interopCheck, &QCheckBox::toggled, this, [this](bool state) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            prefs.enableInterop = state;
            m_controller->updateNetworkSettings(prefs);
        });
    }
    if (auto *interopPort = section->findChild<QSpinBox *>(QStringLiteral("net_interopPort"))) {
        interopPort->setValue(settings.interopPort);
        connect(interopPort, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            prefs.interopPort = static_cast<quint16>(value);
            m_controller->updateNetworkSettings(prefs);
        });
    }
    if (auto *bindCheck = section->findChild<QCheckBox *>(QStringLiteral("net_bindInterface"))) {
        auto *combo = section->findChild<QComboBox *>(QStringLiteral("net_interfaceCombo"));
        bindCheck->setChecked(settings.bindNetworkInterface);
        if (combo) {
            combo->setEnabled(settings.bindNetworkInterface);
            int foundIndex = 0;
            for (int i = 0; i < combo->count(); ++i) {
                if (combo->itemData(i).toString() == settings.boundInterfaceId) {
                    foundIndex = i;
                    break;
                }
            }
            combo->setCurrentIndex(foundIndex);
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, combo](int index) {
                if (!m_controller) {
                    return;
                }
                auto prefs = m_controller->settings().network;
                prefs.boundInterfaceId = combo->itemData(index).toString();
                m_controller->updateNetworkSettings(prefs);
            });
        }
        connect(bindCheck, &QCheckBox::toggled, this, [this, combo](bool state) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            prefs.bindNetworkInterface = state;
            m_controller->updateNetworkSettings(prefs);
            if (combo) {
                combo->setEnabled(state);
            }
        });
    }
    if (m_restrictSubnetCheck) {
        m_restrictSubnetCheck->setChecked(settings.restrictToListedSubnets);
        connect(m_restrictSubnetCheck, &QCheckBox::toggled, this, [this](bool state) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            prefs.restrictToListedSubnets = state;
            m_controller->updateNetworkSettings(prefs);
        });
    }
    refreshBlockedList();
    if (auto *autoRefresh = section->findChild<QCheckBox *>(QStringLiteral("net_autoRefresh"))) {
        autoRefresh->setChecked(settings.autoRefresh);
        connect(autoRefresh, &QCheckBox::toggled, this, [this](bool state) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            prefs.autoRefresh = state;
            m_controller->updateNetworkSettings(prefs);
        });
    }

    if (auto *combo = section->findChild<QComboBox *>(QStringLiteral("net_refreshCombo"))) {
        const QStringList options = {tr("5分钟"), tr("10分钟"), tr("30分钟"), tr("1小时")};
        combo->clear();
        combo->addItems(options);
        int index = 0;
        switch (settings.refreshIntervalMinutes) {
        case 5:
            index = 0;
            break;
        case 10:
            index = 1;
            break;
        case 30:
            index = 2;
            break;
        case 60:
            index = 3;
            break;
        default:
            index = 0;
            break;
        }
        combo->setCurrentIndex(index);
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, combo](int idx) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().network;
            switch (idx) {
            case 0:
                prefs.refreshIntervalMinutes = 5;
                break;
            case 1:
                prefs.refreshIntervalMinutes = 10;
                break;
            case 2:
                prefs.refreshIntervalMinutes = 30;
                break;
            case 3:
                prefs.refreshIntervalMinutes = 60;
                break;
            default:
                break;
            }
            m_controller->updateNetworkSettings(prefs);
        });
    }
}

void SettingsDialog::refreshSubnetList() {
    if (!m_controller || !m_subnetList) {
        return;
    }
    const QString previousToken = m_subnetList->currentItem()
        ? m_subnetList->currentItem()->data(RoleToken).toString()
        : QString();
    m_cachedSubnets = m_controller->configuredSubnets();
    const auto locals = detectLocalSubnets();
    QSet<QString> addedTokens;

    m_subnetList->clear();

    auto addItem = [this, &addedTokens](const QPair<QHostAddress, int> &pair, bool removable, bool isLocal) {
        const QString token = subnetDisplay(pair);
        QString text = token;
        if (isLocal) {
            text = tr("%1（当前网段）").arg(token);
        }
        auto *item = new QListWidgetItem(text, m_subnetList);
        item->setData(RoleToken, token);
        item->setData(RoleRemovable, removable);
        addedTokens.insert(token);
    };

    for (const auto &pair : locals) {
        addItem(pair, false, true);
    }

    for (const auto &pair : std::as_const(m_cachedSubnets)) {
        const QString token = subnetDisplay(pair);
        if (addedTokens.contains(token)) {
            continue;
        }
        addItem(pair, true, false);
    }

    if (m_subnetList->count() == 0) {
        if (m_removeSubnetButton) {
            m_removeSubnetButton->setEnabled(false);
        }
        return;
    }

    bool restored = false;
    if (!previousToken.isEmpty()) {
        for (int i = 0; i < m_subnetList->count(); ++i) {
            auto *item = m_subnetList->item(i);
            if (item->data(RoleToken).toString() == previousToken) {
                m_subnetList->setCurrentRow(i);
                restored = true;
                break;
            }
        }
    }
    if (!restored) {
        m_subnetList->setCurrentRow(0);
    }
}

void SettingsDialog::handleSubnetAdd() {
    if (!m_controller || !m_subnetInput) {
        return;
    }
    QHostAddress network;
    int prefix = -1;
    if (!parseSubnetInput(m_subnetInput->text(), network, prefix)) {
        QMessageBox::warning(this, tr("网段设置"), tr("请输入合法的网段，例如 192.168.1.0/24。"));
        return;
    }
    auto next = m_cachedSubnets;
    const QString token = subnetDisplay(qMakePair(network, prefix));
    for (const auto &pair : std::as_const(next)) {
        if (subnetDisplay(pair) == token) {
            QMessageBox::information(this, tr("网段设置"), tr("该网段已存在，无需重复添加。"));
            return;
        }
    }
    next.append(qMakePair(network, prefix));
    m_controller->setSubnets(next);
    m_subnetInput->clear();
    refreshSubnetList();
}

void SettingsDialog::handleSubnetRemove() {
    if (!m_controller || !m_subnetList) {
        return;
    }
    QListWidgetItem *currentItem = m_subnetList->currentItem();
    if (!currentItem) {
        return;
    }
    if (!currentItem->data(RoleRemovable).toBool()) {
        QMessageBox::warning(this, tr("网段设置"), tr("当前网段用于持续维护联机状态，无法移除。"));
        return;
    }
    const QString token = currentItem->data(RoleToken).toString();
    int targetIndex = -1;
    for (int i = 0; i < m_cachedSubnets.size(); ++i) {
        if (subnetDisplay(m_cachedSubnets.at(i)) == token) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex < 0) {
        return;
    }
    auto next = m_cachedSubnets;
    next.removeAt(targetIndex);
    m_controller->setSubnets(next);
    refreshSubnetList();
}

void SettingsDialog::refreshBlockedList() {
    if (!m_controller || !m_blockList) {
        return;
    }
    const int previousRow = m_blockList->currentRow();
    m_cachedBlockedSubnets = m_controller->blockedSubnets();
    m_blockList->clear();
    for (const auto &pair : std::as_const(m_cachedBlockedSubnets)) {
        m_blockList->addItem(subnetDisplay(pair));
    }
    if (m_blockList->count() == 0) {
        if (m_removeBlockButton) {
            m_removeBlockButton->setEnabled(false);
        }
        return;
    }
    int targetRow = previousRow;
    if (targetRow >= m_blockList->count()) {
        targetRow = m_blockList->count() - 1;
    }
    if (targetRow < 0) {
        targetRow = 0;
    }
    m_blockList->setCurrentRow(targetRow);
}

void SettingsDialog::handleBlockedAdd() {
    if (!m_controller || !m_blockInput) {
        return;
    }
    QHostAddress network;
    int prefix = -1;
    if (!parseSubnetInput(m_blockInput->text(), network, prefix)) {
        QMessageBox::warning(this, tr("网段黑名单"), tr("请输入合法的网段，例如 10.10.0.0/16。"));
        return;
    }
    auto next = m_cachedBlockedSubnets;
    const QString token = subnetDisplay(qMakePair(network, prefix));
    for (const auto &pair : std::as_const(next)) {
        if (subnetDisplay(pair) == token) {
            QMessageBox::information(this, tr("网段黑名单"), tr("该网段已存在，无需重复添加。"));
            return;
        }
    }
    next.append(qMakePair(network, prefix));
    m_controller->setBlockedSubnets(next);
    m_blockInput->clear();
    refreshBlockedList();
}

void SettingsDialog::handleBlockedRemove() {
    if (!m_controller || !m_blockList) {
        return;
    }
    const int row = m_blockList->currentRow();
    if (row < 0 || row >= m_cachedBlockedSubnets.size()) {
        return;
    }
    auto next = m_cachedBlockedSubnets;
    next.removeAt(row);
    m_controller->setBlockedSubnets(next);
    refreshBlockedList();
}

bool SettingsDialog::parseSubnetInput(const QString &text, QHostAddress &network, int &prefixLength) const {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    const int slashIndex = trimmed.indexOf('/');
    const QString ipText = (slashIndex == -1 ? trimmed : trimmed.left(slashIndex)).trimmed();
    const QString prefixText = slashIndex == -1 ? QString() : trimmed.mid(slashIndex + 1).trimmed();
    if (ipText.isEmpty()) {
        return false;
    }
    QHostAddress candidate(ipText);
    if (candidate.isNull()) {
        return false;
    }
    int prefix = -1;
    if (prefixText.isEmpty()) {
        prefix = defaultPrefix(candidate);
    } else {
        bool ok = false;
        prefix = prefixText.toInt(&ok);
        if (!ok) {
            return false;
        }
    }
    const int maxPrefix = candidate.protocol() == QAbstractSocket::IPv6Protocol ? 128 : 32;
    if (prefix < 0 || prefix > maxPrefix) {
        return false;
    }
    network = normalizeNetwork(candidate, prefix);
    prefixLength = prefix;
    return true;
}

QList<QPair<QHostAddress, int>> SettingsDialog::detectLocalSubnets() const {
    QList<QPair<QHostAddress, int>> ranges;
    QSet<QString> seen;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning) ||
            flags.testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            const QHostAddress mask = entry.netmask();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol || mask.protocol() != QAbstractSocket::IPv4Protocol) {
                continue;
            }
            const quint32 networkValue = ip.toIPv4Address() & mask.toIPv4Address();
            quint32 maskValue = mask.toIPv4Address();
            int prefix = 0;
            while (maskValue & 0x80000000u) {
                ++prefix;
                maskValue <<= 1;
            }
            const QHostAddress network(networkValue);
            const QString token = QStringLiteral("%1/%2").arg(network.toString()).arg(prefix);
            if (seen.contains(token) || prefix <= 0) {
                continue;
            }
            seen.insert(token);
            ranges.append(qMakePair(network, prefix));
        }
    }
    return ranges;
}

QString SettingsDialog::detectedSubnetHint() const {
    const auto subnets = detectLocalSubnets();
    if (subnets.isEmpty()) {
        return tr("当前未检测到可用的 IPv4 网段，请确认网络连接正常后再试。");
    }
    QStringList tokens;
    tokens.reserve(subnets.size());
    for (const auto &pair : subnets) {
        tokens.append(subnetDisplay(pair));
    }
    return tr("当前检测到的网段：%1").arg(tokens.join(QStringLiteral("、")));
}

void SettingsDialog::bindNotificationSettings(QWidget *section) {
    if (!section || !m_controller) {
        return;
    }
    const NotificationSettings notify = m_controller->settings().notifications;
    auto bind = [this, section](const QString &name, bool state,
                                std::function<void(NotificationSettings &, bool)> mutate) {
        if (auto *box = section->findChild<QCheckBox *>(name)) {
            box->setChecked(state);
            connect(box, &QCheckBox::toggled, this, [this, mutate](bool checked) {
                if (!m_controller) {
                    return;
                }
                auto prefs = m_controller->settings().notifications;
                mutate(prefs, checked);
                m_controller->updateNotificationSettings(prefs);
            });
        }
    };

    bind(QStringLiteral("notify_selfOnline"), notify.notifySelfOnline,
         [](NotificationSettings &cfg, bool v) { cfg.notifySelfOnline = v; });
    bind(QStringLiteral("notify_peerOnline"), notify.notifyPeerOnline,
         [](NotificationSettings &cfg, bool v) { cfg.notifyPeerOnline = v; });
    bind(QStringLiteral("notify_message"), notify.notifyIncomingMessage,
         [](NotificationSettings &cfg, bool v) { cfg.notifyIncomingMessage = v; });
    bind(QStringLiteral("notify_file"), notify.notifyFileTransfer,
         [](NotificationSettings &cfg, bool v) { cfg.notifyFileTransfer = v; });
    bind(QStringLiteral("notify_mailChange"), notify.notifyMailChange,
         [](NotificationSettings &cfg, bool v) { cfg.notifyMailChange = v; });
    bind(QStringLiteral("notify_muteAll"), notify.muteAll,
         [](NotificationSettings &cfg, bool v) { cfg.muteAll = v; });
    bind(QStringLiteral("notify_groupSound"), notify.playGroupSound,
         [](NotificationSettings &cfg, bool v) { cfg.playGroupSound = v; });
}

void SettingsDialog::bindHotkeySettings(QWidget *section) {
    if (!section || !m_controller) {
        return;
    }
    const HotkeySettings hotkeys = m_controller->settings().hotkeys;
    const struct {
        QString prefix;
        bool enabled;
        QString shortcut;
        std::function<void(HotkeySettings &, bool)> toggle;
        std::function<void(HotkeySettings &, const QString &)> edit;
    } defs[] = {
        {QStringLiteral("hotkey_toggle"), hotkeys.toggleMainWindow, hotkeys.toggleShortcut,
         [](HotkeySettings &cfg, bool v) { cfg.toggleMainWindow = v; },
         [](HotkeySettings &cfg, const QString &text) { cfg.toggleShortcut = text; }},
        {QStringLiteral("hotkey_capture"), hotkeys.captureEnabled, hotkeys.captureShortcut,
         [](HotkeySettings &cfg, bool v) { cfg.captureEnabled = v; },
         [](HotkeySettings &cfg, const QString &text) { cfg.captureShortcut = text; }},
        {QStringLiteral("hotkey_lock"), hotkeys.lockEnabled, hotkeys.lockShortcut,
         [](HotkeySettings &cfg, bool v) { cfg.lockEnabled = v; },
         [](HotkeySettings &cfg, const QString &text) { cfg.lockShortcut = text; }},
        {QStringLiteral("hotkey_storage"), hotkeys.openStorageEnabled, hotkeys.openStorageShortcut,
         [](HotkeySettings &cfg, bool v) { cfg.openStorageEnabled = v; },
         [](HotkeySettings &cfg, const QString &text) { cfg.openStorageShortcut = text; }}
    };

    for (const auto &def : defs) {
        if (auto *box = section->findChild<QCheckBox *>(def.prefix + QStringLiteral("_enable"))) {
            box->setChecked(def.enabled);
            connect(box, &QCheckBox::toggled, this, [this, def](bool state) {
                if (!m_controller) {
                    return;
                }
                auto prefs = m_controller->settings().hotkeys;
                def.toggle(prefs, state);
                m_controller->updateHotkeySettings(prefs);
            });
        }
        if (auto *edit = section->findChild<QLineEdit *>(def.prefix + QStringLiteral("_edit"))) {
            edit->setText(def.shortcut);
            connect(edit, &QLineEdit::editingFinished, this, [this, edit, def]() {
                if (!m_controller) {
                    return;
                }
                auto prefs = m_controller->settings().hotkeys;
                def.edit(prefs, edit->text());
                m_controller->updateHotkeySettings(prefs);
            });
        }
    }
}

void SettingsDialog::bindSecuritySettings(QWidget *section) {
    if (!section || !m_controller) {
        return;
    }
    const SecuritySettings security = m_controller->settings().security;
    if (auto *combo = section->findChild<QComboBox *>(QStringLiteral("security_idleMinutes"))) {
        const QStringList options = {tr("5"), tr("10"), tr("15"), tr("30")};
        combo->clear();
        combo->addItems(options);
        int index = options.indexOf(QString::number(security.idleMinutes));
        if (index < 0) {
            index = 0;
        }
        combo->setCurrentIndex(index);
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, combo](int) {
            if (!m_controller) {
                return;
            }
            auto prefs = m_controller->settings().security;
            prefs.idleMinutes = combo->currentText().toInt();
            m_controller->updateSecuritySettings(prefs);
        });
    }

    if (auto *away = section->findChild<QRadioButton *>(QStringLiteral("security_idleAway"))) {
        away->setChecked(security.idleAction == SecuritySettings::SwitchAway);
        connect(away, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto prefs = m_controller->settings().security;
            prefs.idleAction = SecuritySettings::SwitchAway;
            m_controller->updateSecuritySettings(prefs);
        });
    }
    if (auto *lock = section->findChild<QRadioButton *>(QStringLiteral("security_idleLock"))) {
        lock->setChecked(security.idleAction == SecuritySettings::LockApplication);
        connect(lock, &QRadioButton::toggled, this, [this](bool state) {
            if (!m_controller || !state) {
                return;
            }
            auto prefs = m_controller->settings().security;
            prefs.idleAction = SecuritySettings::LockApplication;
            m_controller->updateSecuritySettings(prefs);
        });
    }

    if (auto *button = section->findChild<QPushButton *>(QStringLiteral("security_passwordBtn"))) {
        connect(button, &QPushButton::clicked, this, [this]() {
            if (!m_controller) {
                return;
            }
            emit m_controller->statusInfo(tr("网络设置入口敬请期待"));
        });
    }

    if (auto *saveBtn = section->findChild<QPushButton *>(QStringLiteral("security_savePass"))) {
        connect(saveBtn, &QPushButton::clicked, this, [this, section]() {
            if (!m_controller) {
                return;
            }
            auto *pass = section->findChild<QLineEdit *>(QStringLiteral("security_pass"));
            auto *confirm = section->findChild<QLineEdit *>(QStringLiteral("security_confirm"));
            if (!pass || !confirm) {
                return;
            }
            if (pass->text().isEmpty() || pass->text() != confirm->text()) {
                QMessageBox::warning(this, tr("密码不一致"), tr("请填写匹配的密码后再保存。"));
                return;
            }
            auto prefs = m_controller->settings().security;
            prefs.lockEnabled = true;
            prefs.passwordHash =
                QString::fromLatin1(QCryptographicHash::hash(pass->text().toUtf8(), QCryptographicHash::Sha256).toHex());
            m_controller->updateSecuritySettings(prefs);
            pass->clear();
            confirm->clear();
            QMessageBox::information(this, tr("已保存"), tr("锁定密码已更新。"));
        });
    }
}

void SettingsDialog::bindMailSettings(QWidget *section) {
    if (!section || !m_controller) {
        return;
    }
    const MailSettings mail = m_controller->settings().mail;
    const QStringList values = {mail.address, mail.imapServer, mail.smtpServer,
                                QString::number(mail.port)};
    for (int i = 0; i < values.size(); ++i) {
        if (auto *edit = section->findChild<QLineEdit *>(QStringLiteral("mail_field_%1").arg(i))) {
            edit->setText(values[i]);
            connect(edit, &QLineEdit::editingFinished, this, [this, section]() {
                if (!m_controller) {
                    return;
                }
                auto prefs = m_controller->settings().mail;
                if (auto *addr = section->findChild<QLineEdit *>(QStringLiteral("mail_field_0"))) {
                    prefs.address = addr->text();
                }
                if (auto *imap = section->findChild<QLineEdit *>(QStringLiteral("mail_field_1"))) {
                    prefs.imapServer = imap->text();
                }
                if (auto *smtp = section->findChild<QLineEdit *>(QStringLiteral("mail_field_2"))) {
                    prefs.smtpServer = smtp->text();
                }
                if (auto *port = section->findChild<QLineEdit *>(QStringLiteral("mail_field_3"))) {
                    prefs.port = port->text().toInt();
                }
                m_controller->updateMailSettings(prefs);
            });
        }
    }

    auto bindCheck = [this, section](const QString &name, bool value,
                                     std::function<void(MailSettings &, bool)> mutate) {
        if (auto *box = section->findChild<QCheckBox *>(name)) {
            box->setChecked(value);
            connect(box, &QCheckBox::toggled, this, [this, mutate](bool state) {
                if (!m_controller) {
                    return;
                }
                auto prefs = m_controller->settings().mail;
                mutate(prefs, state);
                m_controller->updateMailSettings(prefs);
            });
        }
    };
    bindCheck(QStringLiteral("mail_sound"), mail.playSoundOnMail,
              [](MailSettings &cfg, bool v) { cfg.playSoundOnMail = v; });
    bindCheck(QStringLiteral("mail_popup"), mail.popupOnMail,
              [](MailSettings &cfg, bool v) { cfg.popupOnMail = v; });
}

