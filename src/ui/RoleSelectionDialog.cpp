#include "RoleSelectionDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

RoleSelectionDialog::RoleSelectionDialog(ChatController *controller, QWidget *parent)
    : QDialog(parent), m_controller(controller) {
    setWindowTitle(tr("选择沟通角色"));
    setModal(true);
    resize(420, 360);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *intro = new QLabel(tr("每一次沟通前请选择一个角色，用以表达当前的语气与职责。"), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_roleList = new QListWidget(this);
    m_roleList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_roleList, 1);

    auto *signatureLabel = new QLabel(tr("个性签名"), this);
    layout->addWidget(signatureLabel);
    m_signatureEdit = new QLineEdit(this);
    layout->addWidget(m_signatureEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &RoleSelectionDialog::handleAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &RoleSelectionDialog::reject);

    populateRoles();
    if (m_controller) {
        m_signatureEdit->setText(m_controller->signatureText());
    }
}

void RoleSelectionDialog::populateRoles() {
    if (!m_controller || !m_roleList) {
        return;
    }

    m_roleList->clear();
    const QVector<RoleProfile> roles = m_controller->availableRoles();
    for (const RoleProfile &role : roles) {
        auto *item = new QListWidgetItem(QStringLiteral("%1 — %2").arg(role.name, role.signature), m_roleList);
        item->setData(Qt::UserRole, role.id);
        if (role.id == m_controller->activeRole().id) {
            item->setSelected(true);
        }
    }
    if (!m_roleList->selectedItems().isEmpty()) {
        m_roleList->setCurrentItem(m_roleList->selectedItems().first());
    } else if (m_roleList->count() > 0) {
        m_roleList->setCurrentRow(0);
    }
}

void RoleSelectionDialog::handleAccept() {
    if (!m_controller || !m_roleList) {
        reject();
        return;
    }
    auto *current = m_roleList->currentItem();
    if (!current) {
        QMessageBox::warning(this, tr("请选择角色"), tr("请先在左侧列表中选择一个角色。"));
        return;
    }
    const QString roleId = current->data(Qt::UserRole).toString();
    m_controller->setActiveRoleId(roleId);
    m_controller->updateSignatureText(m_signatureEdit->text());
    accept();
}
