#pragma once

#include <QHostAddress>
#include <QString>
#include <QStringList>
#include <QVector>

/*!
 * \brief 角色档案，描述一个可用身份及其展示信息。
 */
struct RoleProfile {
    QString id;
    QString name;
    QString signature;
    QString avatarLetter;
};

/*!
 * \brief 个人资料信息，用于在聊天和资料卡中展示。
 */
struct ProfileDetails {
    QString name;
    QString signature;
    QString gender;
    /*!
     * \brief avatarPath �û��Զ���ͷ��洢·����
     */
    QString avatarPath;
    QString unit = QStringLiteral(u"生物研究院");
    QString department = QStringLiteral(u"神经网络研究室");
    QString phone;
    QString mobile;
    QString email;
    QString version;
    QString ip;
};

/*!
 * \brief 通用偏好设置。
 */
struct GeneralSettings {
    bool autoStart = false;
    bool enableLanUpdate = true;
    bool rejectShake = false;
    bool rejectFeigeAds = false;
    bool enableFeigeGroup = true;
    bool autoMinimize = false;
    bool hideIpAddress = false;
    bool rejectSegmentShare = false;
    bool enableAutoGroup = true;
    bool enableAutoSubGroup = false;
    bool popupOnMessage = false;
    bool flashOnMessage = true;
    QString closeBehavior = QStringLiteral("taskbar");
    QString friendDisplay = QStringLiteral("signature");
};

/*!
 * \brief 网络相关设置。
 */
struct NetworkSettings {
    quint16 searchPort = 9011;
    QString organizationCode;
    bool enableInterop = false;
    quint16 interopPort = 2425;
    bool bindNetworkInterface = false;
    QString boundInterfaceId;
    bool restrictToListedSubnets = false;
    bool autoRefresh = true;
    int refreshIntervalMinutes = 5;
};

/*!
 * \brief 通知提醒设置。
 */
struct NotificationSettings {
    bool notifySelfOnline = false;
    bool notifyPeerOnline = false;
    bool notifyIncomingMessage = true;
    bool notifyFileTransfer = true;
    bool notifyMailChange = false;
    bool muteAll = false;
    bool playGroupSound = false;
};

/*!
 * \brief 快捷键设置。
 */
struct HotkeySettings {
    bool toggleMainWindow = true;
    QString toggleShortcut = QStringLiteral("Ctrl + I");
    bool captureEnabled = true;
    QString captureShortcut = QStringLiteral("Ctrl + Alt + X");
    bool lockEnabled = true;
    QString lockShortcut = QStringLiteral("Ctrl + L");
    bool openStorageEnabled = true;
    QString openStorageShortcut = QStringLiteral("Ctrl + R");
};

/*!
 * \brief 安全设置。
 */
struct SecuritySettings {
    enum IdleAction { SwitchAway, LockApplication };

    bool lockEnabled = false;
    QString passwordHash;
    int idleMinutes = 5;
    IdleAction idleAction = SwitchAway;
};

/*!
 * \brief 邮件相关设置。
 */
struct MailSettings {
    QString address;
    QString imapServer;
    QString smtpServer;
    int port = 465;
    bool playSoundOnMail = false;
    bool popupOnMail = false;
};

/*!
 * \brief 全局应用偏好集合。
 */
struct AppSettings {
    GeneralSettings general;
    NetworkSettings network;
    NotificationSettings notifications;
    HotkeySettings hotkeys;
    SecuritySettings security;
    MailSettings mail;
    QStringList sharedDirectories;
    QString activeRoleId;
    QString signatureText;
    ProfileDetails profile;
};
