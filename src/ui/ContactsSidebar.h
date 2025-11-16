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
    explicit ContactsSidebar(QWidget *parent = nullptr);

    QListView *peerListView() const;
    void setProfileInfo(const QString &displayName, const QString &signature);
    void setAvatarLetter(const QString &letter);
    void setPeerPlaceholderVisible(bool hasPeers);

signals:
    void profileRequested();
    void settingsRequested();
    void statusHintRequested(const QString &text);

private:
    void setupUi();
    QWidget *buildProfileCard(QWidget *parent);
    QWidget *buildSearchRow(QWidget *parent);
    QWidget *buildTabBar(QWidget *parent);
    QWidget *buildPeerSection(QWidget *parent);
    QWidget *createEmptyState(QWidget *parent);
    void applyContactPanelStyle();
    void setActiveTab(int index);

    QListView *m_peerList = nullptr;
    QStackedWidget *m_peerStack = nullptr;
    QWidget *m_emptyState = nullptr;
    QPushButton *m_avatarLabel = nullptr;
    QLabel *m_profileName = nullptr;
    QLabel *m_profileSignature = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QVector<QToolButton *> m_tabButtons;
};

