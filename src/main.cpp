#include "core/ChatController.h"
#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ChatController controller;
    controller.initialize();

    MainWindow window(&controller);
    window.show();

    return app.exec();
}
