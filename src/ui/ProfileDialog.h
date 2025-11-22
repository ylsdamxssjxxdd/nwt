#pragma once

#include "core/ChatController.h"

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QVector>

/*! 
 * \brief 个人资料对话框，单面板展示或编辑当前用户资料。
 */
class ProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileDialog(ChatController *controller, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void handlePrimaryAction();
    void selectAvatarImage();

private:
    void updateEditState(bool editing);
    void saveProfile();
    void applyProfileToFields(const ProfileDetails &profile);
    void refreshAvatarPreview(const QString &overridePath = QString());

    QPointer<ChatController> m_controller;
    QLabel *m_avatarLabel = nullptr;
    QPushButton *m_avatarEditButton = nullptr;
    QPushButton *m_primaryButton = nullptr;
    QVector<QLineEdit *> m_editableFields;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_signatureEdit = nullptr;
    QComboBox *m_genderCombo = nullptr;
    QLineEdit *m_unitEdit = nullptr;
    QLineEdit *m_departmentEdit = nullptr;
    QLineEdit *m_phoneEdit = nullptr;
    QLineEdit *m_mobileEdit = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QLineEdit *m_versionEdit = nullptr;
    QLineEdit *m_ipEdit = nullptr;
    bool m_isEditing = false;
    ProfileDetails m_cachedProfile;
    QString m_pendingAvatarPath;
};
