#include "ProfileDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {
QString emptyPlaceholder(const QString &text) {
    return text.isEmpty() ? QObject::tr("未填写") : text;
}
} // namespace

ProfileDialog::ProfileDialog(ChatController *controller, QWidget *parent)
    : QDialog(parent), m_controller(controller) {
    setWindowTitle(tr("个人信息"));
    resize(380, 520);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto *avatar = new QLabel(this);
    avatar->setObjectName("profileAvatar");
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(96, 96);
    avatar->setText(controller ? controller->profileDetails().name.left(1).toUpper() : tr("E"));
    mainLayout->addWidget(avatar, 0, Qt::AlignHCenter);

    m_stack = new QStackedWidget(this);
    mainLayout->addWidget(m_stack, 1);

    m_viewPanel = new QWidget(this);
    auto *viewLayout = new QFormLayout(m_viewPanel);
    viewLayout->setLabelAlignment(Qt::AlignRight);
    viewLayout->setFormAlignment(Qt::AlignTop);

    const QStringList labels = {tr("姓名："), tr("签名："), tr("性别："), tr("单位："), tr("部门："), tr("电话："),
                                tr("手机："), tr("邮箱："), tr("版本："), tr("IP：")};
    for (const QString &label : labels) {
        auto *value = new QLabel(m_viewPanel);
        value->setObjectName(QStringLiteral("view_%1").arg(labels.indexOf(label)));
        viewLayout->addRow(label, value);
        m_viewLabels.append(value);
    }

    auto *editButton = new QPushButton(tr("编辑资料"), this);
    mainLayout->addWidget(editButton, 0, Qt::AlignRight);

    m_editPanel = new QWidget(this);
    auto *editLayout = new QFormLayout(m_editPanel);
    editLayout->setLabelAlignment(Qt::AlignRight);

    m_nameEdit = new QLineEdit(m_editPanel);
    m_signatureEdit = new QLineEdit(m_editPanel);
    m_genderCombo = new QComboBox(m_editPanel);
    m_genderCombo->addItems({tr("男"), tr("女")});
    m_unitEdit = new QLineEdit(m_editPanel);
    m_departmentEdit = new QLineEdit(m_editPanel);
    m_phoneEdit = new QLineEdit(m_editPanel);
    m_mobileEdit = new QLineEdit(m_editPanel);
    m_emailEdit = new QLineEdit(m_editPanel);
    m_versionEdit = new QLineEdit(m_editPanel);
    m_ipEdit = new QLineEdit(m_editPanel);
    m_versionEdit->setReadOnly(true);
    m_ipEdit->setReadOnly(true);

    editLayout->addRow(tr("姓名："), m_nameEdit);
    editLayout->addRow(tr("签名："), m_signatureEdit);
    editLayout->addRow(tr("性别："), m_genderCombo);
    editLayout->addRow(tr("单位："), m_unitEdit);
    editLayout->addRow(tr("部门："), m_departmentEdit);
    editLayout->addRow(tr("电话："), m_phoneEdit);
    editLayout->addRow(tr("手机："), m_mobileEdit);
    editLayout->addRow(tr("邮箱："), m_emailEdit);
    editLayout->addRow(tr("版本："), m_versionEdit);
    editLayout->addRow(tr("IP："), m_ipEdit);

    m_stack->addWidget(m_viewPanel);
    m_stack->addWidget(m_editPanel);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    mainLayout->addWidget(m_buttonBox);
    m_buttonBox->button(QDialogButtonBox::Save)->setText(tr("保存"));
    m_buttonBox->button(QDialogButtonBox::Close)->setText(tr("关闭"));

    connect(m_buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &ProfileDialog::saveProfile);
    connect(editButton, &QPushButton::clicked, this, &ProfileDialog::enterEditMode);

    if (m_stack->currentIndex() == 0) {
        m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    }

    auto profile = controller ? controller->profileDetails() : ProfileDetails{};
    applyProfileToFields(profile);
}

void ProfileDialog::enterEditMode() {
    if (!m_stack || !m_buttonBox) {
        return;
    }
    m_stack->setCurrentIndex(1);
    m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
}

void ProfileDialog::saveProfile() {
    if (!m_controller) {
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

    if (m_stack && m_buttonBox) {
        m_stack->setCurrentIndex(0);
        m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    }
}

void ProfileDialog::applyProfileToFields(const ProfileDetails &profile) {
    auto setValue = [&](int index, const QString &value) {
        if (index >= 0 && index < m_viewLabels.size() && m_viewLabels[index]) {
            m_viewLabels[index]->setText(emptyPlaceholder(value));
        }
    };
    setValue(0, profile.name);
    setValue(1, profile.signature);
    setValue(2, profile.gender);
    setValue(3, profile.unit);
    setValue(4, profile.department);
    setValue(5, profile.phone);
    setValue(6, profile.mobile);
    setValue(7, profile.email);
    setValue(8, profile.version);
    setValue(9, profile.ip);

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
}

void ProfileDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (m_controller) {
        applyProfileToFields(m_controller->profileDetails());
    }
    if (m_stack && m_buttonBox) {
        m_stack->setCurrentIndex(0);
        m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    }
}
