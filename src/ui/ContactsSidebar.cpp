#include "ContactsSidebar.h"

#include "StyleHelper.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

ContactsSidebar::ContactsSidebar(QWidget *parent) : QFrame(parent) {
    setupUi();
}

void ContactsSidebar::setupUi() {
    setObjectName("contactPanel");
    setMinimumWidth(320);
    setMaximumWidth(380);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(buildProfileCard(this));
    layout->addWidget(buildSearchRow(this));
    layout->addWidget(buildTabBar(this));
    layout->addWidget(buildPeerSection(this), 1);

    applyContactPanelStyle();
}

QListView *ContactsSidebar::peerListView() const {
    return m_peerList;
}

void ContactsSidebar::setProfileInfo(const QString &displayName, const QString &signature) {
    if (m_profileName) {
        m_profileName->setText(displayName.isEmpty() ? tr("EVA-0") : displayName);
    }
    if (m_profileSignature) {
        const QString resolved = signature.isEmpty() ? tr("编辑我的签名") : signature;
        m_profileSignature->setText(formatSignatureText(resolved));
        m_profileSignature->setToolTip(resolved);
    }
}

void ContactsSidebar::setAvatarLetter(const QString &letter) {
    if (m_avatarLabel) {
        const QString resolved = letter.isEmpty() ? tr("E") : letter.left(1).toUpper();
        m_avatarLabel->setText(resolved);
    }
}

void ContactsSidebar::setPeerPlaceholderVisible(bool hasPeers) {
    if (!m_peerStack || !m_peerList || !m_emptyState) {
        return;
    }
    m_peerStack->setCurrentWidget(hasPeers ? static_cast<QWidget *>(m_peerList) : m_emptyState);
}

QWidget *ContactsSidebar::buildProfileCard(QWidget *parent) {
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
    m_profileName = new QLabel(tr("EVA-0"), frame);
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

    const QString defaultSignature = tr("编辑我的签名");
    m_profileSignature = new QLabel(formatSignatureText(defaultSignature), frame);
    m_profileSignature->setObjectName("profileSignature");
    m_profileSignature->setTextFormat(Qt::RichText);
    m_profileSignature->setWordWrap(true);
    m_profileSignature->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_profileSignature->setToolTip(defaultSignature);
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
    auto *settingsButton = createProfileAction(QStyle::SP_FileDialogDetailedView, tr("打开设置"));
    actionLayout->addWidget(refreshButton, 0, Qt::AlignRight);
    actionLayout->addWidget(settingsButton, 0, Qt::AlignRight);
    actionLayout->addStretch(1);
    layout->addLayout(actionLayout);

    connect(statusButton, &QToolButton::clicked, this,
            [this]() { emit statusHintRequested(tr("当前为在线状态，后续可切换")); });
    connect(m_avatarLabel, &QPushButton::clicked, this, &ContactsSidebar::profileRequested);
    connect(refreshButton, &QToolButton::clicked, this, [this]() { emit statusHintRequested(tr("正在刷新状态...")); });
    connect(settingsButton, &QToolButton::clicked, this, &ContactsSidebar::settingsRequested);

    return frame;
}

QWidget *ContactsSidebar::buildSearchRow(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("searchRow");
    frame->setFixedHeight(64);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);

    m_searchEdit = new QLineEdit(frame);
    m_searchEdit->setObjectName("searchField");
    m_searchEdit->setPlaceholderText(tr("搜索联系人、群组、应用"));
    layout->addWidget(m_searchEdit, 1);

    return frame;
}

QWidget *ContactsSidebar::buildTabBar(QWidget *parent) {
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
        {QStyle::SP_MessageBoxInformation, tr("我的联系人")},
        {QStyle::SP_FileDialogDetailedView, tr("我的群组")},
        {QStyle::SP_ComputerIcon, tr("组织架构")},
        {QStyle::SP_FileDialogContentsView, tr("协同办公")}
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

QWidget *ContactsSidebar::buildPeerSection(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("peerSection");

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 0, 16, 12);
    layout->setSpacing(8);

    m_peerStack = new QStackedWidget(frame);
    m_peerList = new QListView(frame);
    m_peerList->setObjectName("peerList");
    m_peerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_peerList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_peerList->setUniformItemSizes(true);

    m_emptyState = createEmptyState(frame);

    m_peerStack->addWidget(m_peerList);
    m_peerStack->addWidget(m_emptyState);
    m_peerStack->setCurrentWidget(m_emptyState);

    layout->addWidget(m_peerStack, 1);

    return frame;
}

QWidget *ContactsSidebar::createEmptyState(QWidget *parent) {
    auto *placeholder = new QWidget(parent);
    auto *layout = new QVBoxLayout(placeholder);
    layout->setContentsMargins(0, 32, 0, 32);
    layout->setSpacing(12);
    auto *iconLabel = new QLabel(tr("通"), placeholder);
    iconLabel->setObjectName("emptyIcon");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(72, 72);
    auto *label = new QLabel(tr("当前没有在线联系人\n稍后再来刷新"), placeholder);
    label->setObjectName("emptyText");
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    layout->addStretch(1);
    layout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    layout->addWidget(label, 0, Qt::AlignHCenter);
    layout->addStretch(1);
    return placeholder;
}

void ContactsSidebar::applyContactPanelStyle() {
    setStyleSheet(UiUtils::loadStyleSheet(QStringLiteral(":/ui/qss/ContactPanel.qss")));
}

void ContactsSidebar::setActiveTab(int index) {
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        if (auto *button = m_tabButtons.at(i)) {
            button->setChecked(i == index);
        }
    }
}

QString ContactsSidebar::formatSignatureText(const QString &signature) const {
    QString escaped = signature.toHtmlEscaped();
    escaped.replace(u'\n', QStringLiteral("<br/>"));
    return QStringLiteral(
               "<div style=\"white-space:pre-wrap; word-break:break-word; overflow-wrap:anywhere;\">%1</div>")
        .arg(escaped);
}
