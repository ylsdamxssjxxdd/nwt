#pragma once

#include <QFrame>

class QLabel;
class QListView;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QToolButton;

/*!
 * \brief ContactsSidebar 负责左侧联系人面板的 UI 构建与简单交互。
 */
class ContactsSidebar : public QFrame {
    Q_OBJECT

public:
    /*!
     * \brief TabIndex 顶部标签按钮索引，与“最近聊天/联系人/群聊”一一对应。
     */
    enum TabIndex { RecentTab = 0, ContactsTab = 1, GroupsTab = 2 };

    explicit ContactsSidebar(QWidget *parent = nullptr);

    QListView *peerListView() const;
    void setProfileInfo(const QString &displayName, const QString &signature);
    void setAvatarLetter(const QString &letter);
    void setPeerPlaceholderVisible(bool hasPeers);

signals:
    void profileRequested();
    void settingsRequested();
    void statusHintRequested(const QString &text);
    /*!
     * \brief tabChanged 当用户切换“最近聊天/联系人/群聊”标签时发出。
     * \param index 参见 TabIndex
     */
    void tabChanged(int index);

private:
    void setupUi();
    QWidget *buildProfileCard(QWidget *parent);
    QWidget *buildSearchRow(QWidget *parent);
    QWidget *buildTabBar(QWidget *parent);
    QWidget *buildPeerSection(QWidget *parent);
    QWidget *createEmptyState(QWidget *parent);
    void applyContactPanelStyle();
    void setActiveTab(int index);
    /*!
     * \brief formatSignatureText ��ǩ������ʽ��������֤�ܹ��Զ��л��У������޸��ַ�����
     * \param signature ԭʼ��ǩ�ı�����
     * \return ���� word-break��overflow-wrap ���Ե� HTML �ַ���
     */
    QString formatSignatureText(const QString &signature) const;

    QListView *m_peerList = nullptr;
    QStackedWidget *m_peerStack = nullptr;
    QWidget *m_emptyState = nullptr;
    QPushButton *m_avatarLabel = nullptr;
    QLabel *m_profileName = nullptr;
    QLabel *m_profileSignature = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QVector<QToolButton *> m_tabButtons;
};
