#pragma once

#include "core/ChatController.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QVector>

class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /*!
     * \brief 构造主窗体并绑定聊天控制器。
     * \param controller 负责联系人与消息的控制器。
     * \param parent 父级窗口。
     */
    explicit MainWindow(ChatController *controller, QWidget *parent = nullptr);

private slots:
    /*! \brief 处理发送按钮或回车事件。 */
    void handleSend();
    /*! \brief 响应联系人列表选择变化。 */
    void handlePeerSelection(const QModelIndex &index);
    /*!
     * \brief 将指定联系人的聊天内容写入聊天区。
     * \param peer 消息来源联系人信息。
     * \param text 消息正文。
     */
    void appendMessage(const PeerInfo &peer, const QString &text);
    /*!
     * \brief 在状态栏区域展示提示。
     * \param text 要展示的文字。
     */
    void showStatus(const QString &text);
    /*! \brief 弹出对话框以添加需要扫描的子网。 */
    void handleAddSubnet();
    /*! \brief 打开设置中心。 */
    void openSettingsDialog();

private:
    /*! \brief 创建窗口整体结构。 */
    void setupUi();
    /*! \brief 构建左侧联系人面板。 */
    QWidget *buildLeftPanel(QWidget *parent);
    /*! \brief 构建右侧聊天面板。 */
    QWidget *buildChatPanel(QWidget *parent);
    /*! \brief 构建顶部单位和操作条。 */
    QWidget *buildTopBanner(QWidget *parent);
    /*! \brief 构建用户档案卡片。 */
    QWidget *buildProfileCard(QWidget *parent);
    /*! \brief 构建搜索区域。 */
    QWidget *buildSearchRow(QWidget *parent);
    /*! \brief 构建功能页签区域。 */
    QWidget *buildTabBar(QWidget *parent);
    /*! \brief 构建联系人列表与占位内容。 */
    QWidget *buildPeerSection(QWidget *parent);
    /*! \brief 构建底部快捷工具栏。 */
    QWidget *buildBottomToolbar(QWidget *parent);
    /*! \brief 生成“暂无联系人”的占位组件。 */
    QWidget *createEmptyState(QWidget *parent);
    /*! \brief 应用联系人面板的统一配色与样式。 */
    void applyContactPanelStyle(QWidget *panel);
    /*! \brief 设置当前选中的页签按钮。 */
    void setActiveTab(int index);
    /*! \brief 根据联系人数量切换占位提示。 */
    void updatePeerPlaceholder();

    ChatController *m_controller = nullptr;
    QWidget *m_contactPanel = nullptr;
    QListView *m_peerList = nullptr;
    QStackedWidget *m_peerStack = nullptr;
    QWidget *m_emptyState = nullptr;
    QLabel *m_avatarLabel = nullptr;
    QLabel *m_profileName = nullptr;
    QLabel *m_profileSignature = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QTextEdit *m_chatLog = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_addSubnetButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QVector<QToolButton *> m_tabButtons;
    QString m_currentPeerId;
    QPointer<SettingsDialog> m_settingsDialog;
};
