#include "SettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QVBoxLayout *sectionLayout(QFrame *section) {
    return qobject_cast<QVBoxLayout *>(section->layout());
}
} // namespace

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
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

    connect(m_navGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &SettingsDialog::switchCategory);
    switchCategory(0);

    setStyleSheet(R"(
        SettingsDialog {
            background: #fff8e7;
        }
        #navPanel {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #f3cc60, stop:1 #f0a92d);
            color: #ffffff;
        }
        #navTitle {
            font-size: 18px;
            font-weight: 600;
            color: #ffffff;
        }
        QToolButton#navButton {
            text-align: left;
            padding: 10px 12px;
            border: none;
            border-radius: 10px;
            color: rgba(255,255,255,0.9);
            font-size: 15px;
            font-weight: 500;
        }
        QToolButton#navButton:hover {
            background: rgba(255, 255, 255, 0.18);
        }
        QToolButton#navButton:checked {
            background: rgba(255, 255, 255, 0.28);
            color: #ffffff;
        }
        #contentArea {
            background: #fffdf6;
        }
        #section {
            background: rgba(255, 255, 255, 0.7);
            border: 1px solid #f4d9a8;
            border-radius: 12px;
            padding: 16px;
        }
        #sectionTitle {
            font-size: 15px;
            color: #b97a0f;
            font-weight: 600;
        }
        #divider {
            color: #e2cfa2;
        }
        QCheckBox, QRadioButton {
            font-size: 14px;
            color: #5e5130;
        }
        QPushButton#actionBtn {
            border: 1px solid #d9d9d9;
            border-radius: 8px;
            background: #f6f6f6;
            padding: 6px 18px;
        }
        QPushButton#actionBtn:hover {
            background: #fff;
        }
        QLineEdit#shortcutEdit {
            border: 1px solid #d9d9d9;
            border-radius: 6px;
            padding: 4px 8px;
            background: #fffdf9;
        }
        #linkLabel {
            color: #1f7bd8;
            text-decoration: underline;
        }
    )");
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

    const QVector<QPair<QString, bool>> leftColumn = {
        {tr("开机自动启动"), true},
        {tr("启用局域网更新"), true},
        {tr("不接收振屏"), false},
        {tr("不给飞鸽飞秋发推荐"), false},
        {tr("开启飞鸽飞秋分组"), false}};
    const QVector<QPair<QString, bool>> rightColumn = {
        {tr("自动最小化到托盘"), false},
        {tr("隐藏自己IP"), false},
        {tr("不接收网段分享"), false},
        {tr("开启自动分组"), true},
        {tr("开启自动分组(联系人)"), false}};

    for (int i = 0; i < leftColumn.size(); ++i) {
        auto *box = new QCheckBox(leftColumn[i].first, common);
        box->setChecked(leftColumn[i].second);
        grid->addWidget(box, i, 0);
    }
    for (int i = 0; i < rightColumn.size(); ++i) {
        auto *box = new QCheckBox(rightColumn[i].first, common);
        box->setChecked(rightColumn[i].second);
        grid->addWidget(box, i, 1);
    }
    commonLayout->addLayout(grid);
    layout->addWidget(common);

    auto *messageSection = createSection(tr("接到新消息时"));
    auto *messageLayout = sectionLayout(messageSection);
    auto *messageRow = new QHBoxLayout();
    auto *autoPopup = new QRadioButton(tr("自动弹出"), messageSection);
    auto *flashHint = new QRadioButton(tr("闪烁提示"), messageSection);
    flashHint->setChecked(true);
    messageRow->addWidget(autoPopup);
    messageRow->addWidget(flashHint);
    messageRow->addStretch();
    messageLayout->addLayout(messageRow);
    layout->addWidget(messageSection);

    auto *closeSection = createSection(tr("关闭主面板时"));
    auto *closeLayout = sectionLayout(closeSection);
    auto *closeRow = new QHBoxLayout();
    auto *exitApp = new QRadioButton(tr("退出程序"), closeSection);
    auto *minimizeTask = new QRadioButton(tr("最小化到任务栏"), closeSection);
    minimizeTask->setChecked(true);
    closeRow->addWidget(exitApp);
    closeRow->addWidget(minimizeTask);
    closeRow->addStretch();
    closeLayout->addLayout(closeRow);
    layout->addWidget(closeSection);

    auto *friendSection = createSection(tr("好友信息显示"));
    auto *friendLayout = sectionLayout(friendSection);
    auto *friendRow = new QHBoxLayout();
    auto *showSignature = new QRadioButton(tr("显示签名"), friendSection);
    auto *showIp = new QRadioButton(tr("显示IP"), friendSection);
    showSignature->setChecked(true);
    friendRow->addWidget(showSignature);
    friendRow->addWidget(showIp);
    friendRow->addStretch();
    friendLayout->addLayout(friendRow);
    layout->addWidget(friendSection);

    layout->addStretch(1);
    return page;
}

QWidget *SettingsDialog::createNetworkPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *networkSection = createSection(tr("网络设置"));
    auto *networkLayout = sectionLayout(networkSection);
    auto *netButton = new QPushButton(tr("网络设置"), networkSection);
    netButton->setObjectName("actionBtn");
    networkLayout->addWidget(netButton, 0, Qt::AlignLeft);
    layout->addWidget(networkSection);

    auto *segmentSection = createSection(tr("其他网段联系人"));
    auto *segmentLayout = sectionLayout(segmentSection);
    const QVector<QString> buttons = {tr("网段设置"), tr("网段黑名单")};
    for (const QString &text : buttons) {
        auto *btn = new QPushButton(text, segmentSection);
        btn->setObjectName("actionBtn");
        segmentLayout->addWidget(btn, 0, Qt::AlignLeft);
    }
    layout->addWidget(segmentSection);

    auto *remoteSection = createSection(tr("远程密码"));
    auto *remoteLayout = sectionLayout(remoteSection);
    auto *passwordBtn = new QPushButton(tr("设置密码"), remoteSection);
    passwordBtn->setObjectName("actionBtn");
    remoteLayout->addWidget(passwordBtn, 0, Qt::AlignLeft);
    layout->addWidget(remoteSection);

    auto *refreshSection = createSection(tr("自动刷新"));
    auto *refreshLayout = sectionLayout(refreshSection);
    auto *refreshRow = new QHBoxLayout();
    auto *autoRefresh = new QCheckBox(tr("开启自动刷新"), refreshSection);
    autoRefresh->setChecked(true);
    auto *combo = new QComboBox(refreshSection);
    combo->addItems({tr("5分钟"), tr("10分钟"), tr("30分钟"), tr("1小时")});
    refreshRow->addWidget(autoRefresh);
    refreshRow->addStretch();
    refreshLayout->addLayout(refreshRow);

    auto *comboRow = new QHBoxLayout();
    auto *label = new QLabel(tr("刷新时间："), refreshSection);
    comboRow->addWidget(label);
    comboRow->addWidget(combo, 0);
    comboRow->addStretch();
    refreshLayout->addLayout(comboRow);

    layout->addWidget(refreshSection);
    layout->addStretch(1);
    return page;
}

QWidget *SettingsDialog::createNotificationPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    auto *notifySection = createSection(tr("哪些要通知？"));
    auto *notifyLayout = sectionLayout(notifySection);
    const QVector<QString> notifications = {
        tr("当线上联系人上线时，提醒其他联系人"),
        tr("当其他联系人上线时，显示通知窗口"),
        tr("收到即时消息/文件时，显示通知窗口"),
        tr("文件传输成功时，显示通知窗口"),
        tr("邮件数量变化时，显示通知窗口")};
    for (const QString &text : notifications) {
        notifyLayout->addWidget(new QCheckBox(text, notifySection));
    }
    layout->addWidget(notifySection);

    auto *soundSection = createSection(tr("提示声音"));
    auto *soundLayout = sectionLayout(soundSection);
    auto *muteRow = new QHBoxLayout();
    auto *disableSound = new QCheckBox(tr("关闭所有提示声音"), soundSection);
    auto *soundLink = new QLabel(tr("声音目录"), soundSection);
    soundLink->setObjectName("linkLabel");
    muteRow->addWidget(disableSound);
    muteRow->addWidget(soundLink);
    muteRow->addStretch();
    soundLayout->addLayout(muteRow);

    soundLayout->addWidget(new QCheckBox(tr("收到每条群消息都播放提示声音"), soundSection));
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
        {tr("显示/隐藏主窗口"), QStringLiteral("Ctrl + I")},
        {tr("截图"), QStringLiteral("Ctrl + Alt + X")},
        {tr("锁定内网通"), QStringLiteral("Ctrl + L")},
        {tr("打开文件存储目录"), QStringLiteral("Ctrl + R")}};
    for (const auto &pair : shortcuts) {
        auto *row = new QHBoxLayout();
        auto *box = new QCheckBox(pair.first, hotkeySection);
        box->setChecked(true);
        row->addWidget(box);
        auto *edit = new QLineEdit(pair.second, hotkeySection);
        edit->setObjectName("shortcutEdit");
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
    passwordButton->setObjectName("actionBtn");
    lockLayout->addWidget(passwordButton, 0, Qt::AlignLeft);
    layout->addWidget(lockSection);

    auto *idleSection = createSection(tr("鼠标键盘无动作"));
    auto *idleLayout = sectionLayout(idleSection);
    auto *idleRow = new QHBoxLayout();
    auto *spinLabel = new QLabel(tr("分钟后"), idleSection);
    auto *combo = new QComboBox(idleSection);
    combo->addItems({tr("5"), tr("10"), tr("15"), tr("30")});
    idleRow->addWidget(combo, 0);
    idleRow->addWidget(spinLabel);
    idleRow->addStretch();
    idleLayout->addLayout(idleRow);

    auto *statusRow = new QHBoxLayout();
    auto *away = new QRadioButton(tr("切换为“离开”"), idleSection);
    away->setChecked(true);
    auto *lock = new QRadioButton(tr("锁定内网通"), idleSection);
    statusRow->addWidget(away);
    statusRow->addWidget(lock);
    statusRow->addStretch();
    idleLayout->addLayout(statusRow);

    layout->addWidget(idleSection);
    layout->addStretch(1);
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
    passEdit->setEchoMode(QLineEdit::Password);
    auto *confirmLabel = new QLabel(tr("确认密码："), lockSection);
    auto *confirmEdit = new QLineEdit(lockSection);
    confirmEdit->setEchoMode(QLineEdit::Password);
    form->addWidget(passLabel, 0, 0);
    form->addWidget(passEdit, 0, 1);
    form->addWidget(confirmLabel, 1, 0);
    form->addWidget(confirmEdit, 1, 1);
    lockLayout->addLayout(form);

    auto *btnRow = new QHBoxLayout();
    auto *apply = new QPushButton(tr("保存密码"), lockSection);
    apply->setObjectName("actionBtn");
    btnRow->addWidget(apply, 0, Qt::AlignLeft);
    btnRow->addStretch();
    lockLayout->addLayout(btnRow);
    layout->addWidget(lockSection);

    auto *tip = new QLabel(tr("提示：密码信息仅保存在本机，可在主界面从新建图标处进行快速锁定。"), page);
    tip->setWordWrap(true);
    layout->addWidget(tip);
    layout->addStretch(1);
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
        mailForm->addWidget(label, i, 0);
        mailForm->addWidget(edit, i, 1);
    }
    mailLayout->addLayout(mailForm);
    layout->addWidget(mailSection);

    auto *notifySection = createSection(tr("邮件通知"));
    auto *notifyLayout = sectionLayout(notifySection);
    notifyLayout->addWidget(new QCheckBox(tr("新邮件到达时播报提示音"), notifySection));
    notifyLayout->addWidget(new QCheckBox(tr("新邮件到达时显示通知窗口"), notifySection));
    layout->addWidget(notifySection);

    layout->addStretch(1);
    return page;
}

void SettingsDialog::switchCategory(int index) {
    if (!m_stack) {
        return;
    }
    m_stack->setCurrentIndex(index);
}
