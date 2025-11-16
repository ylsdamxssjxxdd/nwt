#include "ProfileDialog.h"
#include "StyleHelper.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace {
QLabel *createFieldLabel(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("profileFieldLabel"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}
} // namespace

ProfileDialog::ProfileDialog(ChatController *controller, QWidget *parent)
    : QDialog(parent), m_controller(controller) {
    setWindowTitle(tr("个人信息"));
    resize(420, 640);
    setAttribute(Qt::WA_StyledBackground, true);

    setStyleSheet(UiUtils::loadStyleSheet(QStringLiteral(":/ui/qss/ProfileDialog.qss")));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(18);

    auto *headerWidget = new QWidget(this);
    headerWidget->setObjectName("profileHeader");
    headerWidget->setFixedHeight(180);
    auto *headerLayout = new QGridLayout(headerWidget);
    headerLayout->setContentsMargins(24, 24, 24, 24);
    headerLayout->setHorizontalSpacing(16);
    headerLayout->setVerticalSpacing(8);

    m_avatarLabel = new QLabel(headerWidget);
    m_avatarLabel->setObjectName("profileAvatar");
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setFixedSize(96, 96);
    m_avatarLabel->setCursor(Qt::PointingHandCursor);
    m_avatarLabel->setAttribute(Qt::WA_Hover);
    m_avatarLabel->installEventFilter(this);
    headerLayout->addWidget(m_avatarLabel, 0, 0, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);

    auto *titleLabel = new QLabel(tr("个人信息"), headerWidget);
    titleLabel->setObjectName("headerTitle");
    headerLayout->addWidget(titleLabel, 0, 1, 1, 1, Qt::AlignLeft | Qt::AlignBottom);

    auto *subtitleLabel = new QLabel(tr("完善资料，便于同事快速了解你"), headerWidget);
    subtitleLabel->setObjectName("headerSubtitle");
    headerLayout->addWidget(subtitleLabel, 1, 1, 1, 1, Qt::AlignLeft | Qt::AlignTop);

    m_primaryButton = new QPushButton(tr("编辑"), headerWidget);
    m_primaryButton->setObjectName("primaryActionButton");
    m_primaryButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(m_primaryButton, 0, 2, 2, 1, Qt::AlignTop | Qt::AlignRight);
    mainLayout->addWidget(headerWidget);

    auto *cardWidget = new QWidget(this);
    cardWidget->setObjectName("profileCard");
    auto *cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    auto *formPanel = new QWidget(cardWidget);
    auto *formLayout = new QFormLayout(formPanel);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFormAlignment(Qt::AlignTop);
    formLayout->setHorizontalSpacing(24);
    formLayout->setVerticalSpacing(12);

    auto createLineEdit = [&](const QString &placeholder, bool editable) {
        auto *edit = new QLineEdit(formPanel);
        edit->setPlaceholderText(placeholder);
        if (editable) {
            edit->setReadOnly(true);
            m_editableFields.append(edit);
        } else {
            edit->setReadOnly(true);
        }
        return edit;
    };

    m_nameEdit = createLineEdit(tr("请输入姓名"), true);
    m_signatureEdit = createLineEdit(tr("设置个性签名"), true);
    m_genderCombo = new QComboBox(formPanel);
    m_genderCombo->addItems({tr("男"), tr("女")});
    m_genderCombo->setEnabled(false);
    m_genderCombo->setCursor(Qt::PointingHandCursor);
    m_unitEdit = createLineEdit(tr("请输入所在单位"), true);
    m_departmentEdit = createLineEdit(tr("请输入所属部门"), true);
    m_phoneEdit = createLineEdit(tr("填写座机或直线"), true);
    m_mobileEdit = createLineEdit(tr("填写常用手机号"), true);
    m_emailEdit = createLineEdit(tr("填写联系邮箱"), true);
    m_versionEdit = createLineEdit(tr("由系统自动填写"), false);
    m_ipEdit = createLineEdit(tr("由系统自动填写"), false);

    auto addEditRow = [&](const QString &labelText, QWidget *editor) {
        formLayout->addRow(createFieldLabel(labelText, formPanel), editor);
    };

    addEditRow(tr("姓名："), m_nameEdit);
    addEditRow(tr("签名："), m_signatureEdit);
    addEditRow(tr("性别："), m_genderCombo);
    addEditRow(tr("单位："), m_unitEdit);
    addEditRow(tr("部门："), m_departmentEdit);
    addEditRow(tr("电话："), m_phoneEdit);
    addEditRow(tr("手机："), m_mobileEdit);
    addEditRow(tr("邮箱："), m_emailEdit);
    addEditRow(tr("版本："), m_versionEdit);
    addEditRow(tr("IP："), m_ipEdit);

    cardLayout->addWidget(formPanel);
    mainLayout->addWidget(cardWidget, 1);

    connect(m_primaryButton, &QPushButton::clicked, this, &ProfileDialog::handlePrimaryAction);

    auto profile = controller ? controller->profileDetails() : ProfileDetails{};
    applyProfileToFields(profile);
    updateEditState(false);
}

void ProfileDialog::handlePrimaryAction() {
    if (m_isEditing) {
        saveProfile();
    } else {
        updateEditState(true);
        if (m_nameEdit) {
            m_nameEdit->setFocus();
            m_nameEdit->selectAll();
        }
    }
}

void ProfileDialog::updateEditState(bool editing) {
    m_isEditing = editing;
    for (auto *edit : std::as_const(m_editableFields)) {
        if (edit) {
            edit->setReadOnly(!editing);
        }
    }
    if (m_genderCombo) {
        m_genderCombo->setEnabled(editing);
    }
    if (m_primaryButton) {
        m_primaryButton->setText(editing ? tr("保存") : tr("编辑"));
    }
}

void ProfileDialog::saveProfile() {
    if (!m_controller) {
        updateEditState(false);
        return;
    }

    ProfileDetails details = m_controller->profileDetails();
    details.name = m_nameEdit->text().trimmed();
    details.signature = m_signatureEdit->text().trimmed();
    details.gender = m_genderCombo->currentText();
    details.unit = m_unitEdit->text().trimmed();
    details.department = m_departmentEdit->text().trimmed();
    details.phone = m_phoneEdit->text().trimmed();
    details.mobile = m_mobileEdit->text().trimmed();
    details.email = m_emailEdit->text().trimmed();
    details.version = m_versionEdit->text();
    details.ip = m_ipEdit->text();

    m_controller->updateProfileDetails(details);
    applyProfileToFields(details);
    updateEditState(false);
}

void ProfileDialog::applyProfileToFields(const ProfileDetails &profile) {
    if (m_nameEdit) {
        m_nameEdit->setText(profile.name);
    }
    if (m_signatureEdit) {
        m_signatureEdit->setText(profile.signature);
    }
    if (m_genderCombo) {
        int genderIndex = m_genderCombo->findText(profile.gender);
        m_genderCombo->setCurrentIndex(genderIndex == -1 ? 0 : genderIndex);
    }
    if (m_unitEdit) {
        m_unitEdit->setText(profile.unit);
    }
    if (m_departmentEdit) {
        m_departmentEdit->setText(profile.department);
    }
    if (m_phoneEdit) {
        m_phoneEdit->setText(profile.phone);
    }
    if (m_mobileEdit) {
        m_mobileEdit->setText(profile.mobile);
    }
    if (m_emailEdit) {
        m_emailEdit->setText(profile.email);
    }
    if (m_versionEdit) {
        m_versionEdit->setText(profile.version);
    }
    if (m_ipEdit) {
        m_ipEdit->setText(profile.ip);
    }
    if (m_avatarLabel) {
        QString letter = profile.name.trimmed();
        letter = letter.isEmpty() ? QStringLiteral("E") : letter.left(1).toUpper();
        m_avatarLabel->setText(letter);
    }
}

void ProfileDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (m_controller) {
        applyProfileToFields(m_controller->profileDetails());
    }
    updateEditState(false);
}

bool ProfileDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_avatarLabel) {
        if (event->type() == QEvent::MouseButtonRelease) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton && !m_isEditing) {
                handlePrimaryAction();
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}
