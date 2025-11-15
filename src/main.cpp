#include "core/ChatController.h"
#include "ui/MainWindow.h"
#include "ui/RoleSelectionDialog.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ChatController controller;
    controller.initialize();

    RoleSelectionDialog roleDialog(&controller);
    if (roleDialog.exec() != QDialog::Accepted && controller.requiresRoleSelection()) {
        return 0;
    }

    MainWindow window(&controller);
    window.show();

    return app.exec();
}
