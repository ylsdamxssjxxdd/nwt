#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAbstractSocket>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QInputDialog>
#include <QItemSelectionModel>
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

    m_peerList->setModel(m_controller->peerDirectory());
    connect(m_peerList->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &) { handlePeerSelection(current); });

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::handleSend);
    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::handleSend);
    connect(m_controller, &ChatController::chatMessageReceived, this, &MainWindow::appendMessage);
    connect(m_controller, &ChatController::statusInfo, this, &MainWindow::showStatus);
    connect(m_controller, &ChatController::controllerWarning, this, &MainWindow::showStatus);
    connect(m_addSubnetButton, &QPushButton::clicked, this, &MainWindow::handleAddSubnet);
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);

    m_peerList = new QListView(central);
    m_peerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_peerList->setMinimumWidth(180);
    auto *leftColumn = new QVBoxLayout();
    leftColumn->addWidget(m_peerList, 1);
    m_addSubnetButton = new QPushButton(tr("添加网段"), central);
    leftColumn->addWidget(m_addSubnetButton);
    layout->addLayout(leftColumn);

    auto *chatColumn = new QVBoxLayout();
    m_chatLog = new QTextEdit(central);
    m_chatLog->setReadOnly(true);
    chatColumn->addWidget(m_chatLog);

    auto *inputRow = new QHBoxLayout();
    m_input = new QLineEdit(central);
    m_sendButton = new QPushButton(tr("发送"), central);
    inputRow->addWidget(m_input);
    inputRow->addWidget(m_sendButton);
    chatColumn->addLayout(inputRow);

    m_statusLabel = new QLabel(tr("未连接"), central);
    chatColumn->addWidget(m_statusLabel);

    layout->addLayout(chatColumn, 1);
    setCentralWidget(central);
    setWindowTitle(tr("内网聊天"));
    resize(760, 480);
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
        showStatus(tr("请选择一个联系人"));
        return;
    }

    m_controller->sendMessageToPeer(m_currentPeerId, text);
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_chatLog->append(QStringLiteral("[%1] 我: %2").arg(timestamp, text));
    m_input->clear();
}

void MainWindow::handlePeerSelection(const QModelIndex &index) {
    m_currentPeerId = index.data(PeerDirectory::IdRole).toString();
    const QString display = index.data(Qt::DisplayRole).toString();
    showStatus(tr("选中 %1").arg(display));
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
    const QString cidr = QInputDialog::getText(this, tr("添加网段"),
                                              tr("请输入 CIDR（示例 192.168.1.0/24）："),
                                              QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }

    const QString trimmed = cidr.trimmed();
    if (trimmed.isEmpty()) {
        showStatus(tr("请输入网段或 IP"));
        return;
    }

    const int slashIndex = trimmed.indexOf('/');
    const QString ipText = (slashIndex == -1 ? trimmed : trimmed.left(slashIndex)).trimmed();
    const QString prefixText = slashIndex == -1 ? QString() : trimmed.mid(slashIndex + 1).trimmed();

    if (ipText.isEmpty()) {
        showStatus(tr("格式应为 192.168.1.0/24"));
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
            showStatus(tr("掩码无效: %1").arg(prefixText));
            return;
        }
    }

    const int maxPrefix = network.protocol() == QAbstractSocket::IPv6Protocol ? 128 : 32;
    if (prefixLength < 0 || prefixLength > maxPrefix) {
        showStatus(tr("掩码范围应为 0-%1").arg(maxPrefix));
        return;
    }

    const QHostAddress normalized = normalizeNetwork(network, prefixLength);
    m_controller->addSubnet(normalized, prefixLength);
    if (slashIndex == -1) {
        showStatus(tr("已按默认掩码添加 %1/%2").arg(normalized.toString()).arg(prefixLength));
    } else {
        showStatus(tr("已添加网段 %1/%2").arg(normalized.toString()).arg(prefixLength));
    }
}
