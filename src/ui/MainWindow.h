#pragma once

#include "core/ChatController.h"

#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ChatController *controller, QWidget *parent = nullptr);

private slots:
    void handleSend();
    void handlePeerSelection(const QModelIndex &index);
    void appendMessage(const PeerInfo &peer, const QString &text);
    void showStatus(const QString &text);
    void handleAddSubnet();

private:
    void setupUi();

    ChatController *m_controller = nullptr;
    QListView *m_peerList = nullptr;
    QTextEdit *m_chatLog = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_addSubnetButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QString m_currentPeerId;
};
