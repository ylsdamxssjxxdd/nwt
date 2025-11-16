#include "ChatPanel.h"

#include "StyleHelper.h"

#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

ChatPanel::ChatPanel(QWidget *parent) : QFrame(parent) {
    setupUi();
}

void ChatPanel::setupUi() {
    setObjectName("chatPanel");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QFrame(this);
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

    m_chatPresenceLabel = new QLabel(tr("尚未连接"), header);
    m_chatPresenceLabel->setObjectName("chatPresence");
    headerInfo->addWidget(m_chatPresenceLabel);
    headerLayout->addLayout(headerInfo);
    headerLayout->addStretch();

    layout->addWidget(header);

    auto *body = new QFrame(this);
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

    m_chatEmptyLabel = new QLabel(tr("选择联系人后即可开始对话"), m_messageViewport);
    m_chatEmptyLabel->setObjectName("chatEmpty");
    m_chatEmptyLabel->setAlignment(Qt::AlignCenter);
    m_chatEmptyLabel->setWordWrap(true);
    m_messageLayout->addWidget(m_chatEmptyLabel, 0, Qt::AlignCenter);
    m_messageLayout->addStretch(1);

    m_messageScroll->setWidget(m_messageViewport);
    bodyLayout->addWidget(m_messageScroll);
    layout->addWidget(body, 1);

    auto *inputArea = new QFrame(this);
    inputArea->setObjectName("chatInputArea");
    auto *inputLayout = new QVBoxLayout(inputArea);
    inputLayout->setContentsMargins(24, 12, 24, 10);
    inputLayout->setSpacing(8);

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
    toolRow->addWidget(buildToolButton(tr("截图")));
    toolRow->addWidget(buildToolButton(tr("收藏")));
    toolRow->addWidget(buildToolButton(tr("更多")));

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

    auto *statusBar = new QFrame(this);
    statusBar->setObjectName("chatStatusBar");
    auto *statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(32, 8, 32, 8);
    statusLayout->setSpacing(8);
    m_statusLabel = new QLabel(tr("未连接"), statusBar);
    m_statusLabel->setObjectName("statusLabel");
    statusLayout->addWidget(m_statusLabel);
    layout->addWidget(statusBar, 0);

    setStyleSheet(UiUtils::loadStyleSheet(QStringLiteral(":/ui/qss/ChatPanel.qss")));

    connect(m_sendButton, &QPushButton::clicked, this, &ChatPanel::sendRequested);
    connect(m_fileButton, &QPushButton::clicked, this, &ChatPanel::fileSendRequested);
    connect(m_shareButton, &QPushButton::clicked, this, &ChatPanel::shareCenterRequested);
}

void ChatPanel::setChatHeader(const QString &title, const QString &presence) {
    if (m_chatNameLabel) {
        m_chatNameLabel->setText(title.isEmpty() ? tr("选择联系人") : title);
    }
    if (m_chatPresenceLabel) {
        const QString fallback = title.isEmpty() ? tr("尚未连接") : tr("已连接，可立即开始对话");
        m_chatPresenceLabel->setText(presence.isEmpty() ? fallback : presence);
    }
}

void ChatPanel::appendOutgoingMessage(const QString &timestamp, const QString &sender, const QString &text) {
    appendChatBubble(timestamp, sender, text, true);
}

void ChatPanel::appendIncomingMessage(const QString &timestamp, const QString &sender, const QString &text) {
    appendChatBubble(timestamp, sender, text, false);
}

void ChatPanel::appendTimelineHint(const QString &timestamp, const QString &tag) {
    ensureChatAreaForNewEntry();
    if (!m_messageLayout) {
        return;
    }
    auto *hint = new QLabel(m_messageViewport);
    hint->setObjectName("timelineLabel");
    const QString content = tag.isEmpty() ? timestamp : QStringLiteral("%1  %2").arg(timestamp, tag);
    hint->setText(content);
    hint->setAlignment(Qt::AlignCenter);
    hint->setTextFormat(Qt::PlainText);
    m_messageLayout->addWidget(hint, 0, Qt::AlignCenter);
    m_messageLayout->addStretch(1);
    scrollToLatestMessage();
}

QString ChatPanel::inputText() const {
    return m_inputEdit ? m_inputEdit->toPlainText().trimmed() : QString();
}

void ChatPanel::clearInput() {
    if (m_inputEdit) {
        m_inputEdit->clear();
    }
}

void ChatPanel::focusInput() {
    if (m_inputEdit) {
        m_inputEdit->setFocus();
    }
}

void ChatPanel::setStatusText(const QString &text) {
    if (m_statusLabel) {
        m_statusLabel->setText(text);
    }
}

bool ChatPanel::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_inputEdit && event && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers().testFlag(Qt::ShiftModifier)) {
                return QFrame::eventFilter(watched, event);
            }
            emit sendRequested();
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void ChatPanel::appendChatBubble(const QString &timestamp, const QString &sender, const QString &text, bool outgoing) {
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

void ChatPanel::ensureChatAreaForNewEntry() {
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

void ChatPanel::scrollToLatestMessage() const {
    if (!m_messageScroll) {
        return;
    }
    if (auto *bar = m_messageScroll->verticalScrollBar()) {
        bar->setValue(bar->maximum());
    }
}
