#pragma once

#include <QFrame>

class QLabel;
class QScrollArea;
class QVBoxLayout;
class QTextEdit;
class QPushButton;

/*!
 * \brief ChatPanel 管理聊天内容区与输入区。
 */
class ChatPanel : public QFrame {
    Q_OBJECT

public:
    explicit ChatPanel(QWidget *parent = nullptr);

    void setChatHeader(const QString &title, const QString &presence);
    void appendOutgoingMessage(const QString &timestamp, const QString &sender, const QString &text);
    void appendIncomingMessage(const QString &timestamp, const QString &sender, const QString &text);
    void appendTimelineHint(const QString &timestamp, const QString &tag);
    QString inputText() const;
    void clearInput();
    void focusInput();
    void setStatusText(const QString &text);

signals:
    void sendRequested();
    void fileSendRequested();
    void shareCenterRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUi();
    void appendChatBubble(const QString &timestamp, const QString &sender, const QString &text, bool outgoing);
    void ensureChatAreaForNewEntry();
    void scrollToLatestMessage() const;

    QLabel *m_chatNameLabel = nullptr;
    QLabel *m_chatPresenceLabel = nullptr;
    QLabel *m_chatEmptyLabel = nullptr;
    QScrollArea *m_messageScroll = nullptr;
    QWidget *m_messageViewport = nullptr;
    QVBoxLayout *m_messageLayout = nullptr;
    QTextEdit *m_inputEdit = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_fileButton = nullptr;
    QPushButton *m_shareButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};
