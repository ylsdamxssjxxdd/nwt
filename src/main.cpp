#include "core/ChatController.h"
#include "core/LanguageManager.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QProcessEnvironment>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto &lang = LanguageManager::instance();
    lang.initialize();
    const QString envLanguage = qEnvironmentVariable("NWT_LANG");
    const auto targetLanguage = envLanguage.isEmpty()
                                    ? LanguageManager::languageForLocale(QLocale::system())
                                    : LanguageManager::languageFromCode(envLanguage);
    if (!lang.switchLanguage(targetLanguage)) {
        lang.switchLanguage(LanguageManager::Language::ChineseSimplified);
    }

    ChatController controller;
    controller.initialize();

    MainWindow window(&controller);
    window.show();

    return app.exec();
}
