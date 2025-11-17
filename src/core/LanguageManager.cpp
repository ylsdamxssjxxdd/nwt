#include "LanguageManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QReadLocker>
#include <QSettings>
#include <QStringList>
#include <QWriteLocker>
#include <utility>

LanguageManager &LanguageManager::instance() {
    static LanguageManager manager;
    return manager;
}

QString LanguageManager::text(int id, const QString &fallback) {
    return LanguageManager::instance().lookupText(id, fallback);
}

LanguageManager::LanguageManager(QObject *parent) : QObject(parent) {
}

bool LanguageManager::initialize(const QString &customLanguageDir) {
    QString resolved = customLanguageDir;
    if (resolved.isEmpty()) {
        resolved = detectLanguageDirectory();
    } else {
        QDir dir(resolved);
        if (!dir.exists()) {
            qWarning() << "Language directory does not exist:" << resolved;
            return false;
        }
        resolved = dir.absolutePath();
    }

    if (resolved.isEmpty()) {
        qWarning() << "Language directory could not be located.";
        return false;
    }

    QWriteLocker locker(&m_lock);
    m_languageDir = resolved;
    return true;
}

bool LanguageManager::switchLanguage(Language language) {
    QString dir;
    {
        QReadLocker locker(&m_lock);
        dir = m_languageDir;
    }
    if (dir.isEmpty()) {
        if (!initialize()) {
            return false;
        }
        QReadLocker locker(&m_lock);
        dir = m_languageDir;
        if (dir.isEmpty()) {
            return false;
        }
    }

    const QString filePath = dir + QLatin1Char('/') + languageFileName(language);
    QHash<int, QString> buffer;
    if (!loadFile(filePath, buffer)) {
        return false;
    }

    {
        QWriteLocker locker(&m_lock);
        m_translations = std::move(buffer);
        m_currentLanguage = language;
    }

    emit languageChanged(language);
    return true;
}

LanguageManager::Language LanguageManager::currentLanguage() const {
    QReadLocker locker(&m_lock);
    return m_currentLanguage;
}

QString LanguageManager::languageDirectory() const {
    QReadLocker locker(&m_lock);
    return m_languageDir;
}

QString LanguageManager::codeFor(Language language) {
    switch (language) {
    case Language::ChineseSimplified:
        return QStringLiteral("zh_CN");
    case Language::English:
        return QStringLiteral("en_US");
    }
    return QStringLiteral("zh_CN");
}

LanguageManager::Language LanguageManager::languageFromCode(const QString &code) {
    const QString normalized = code.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("en"))) {
        return Language::English;
    }
    return Language::ChineseSimplified;
}

LanguageManager::Language LanguageManager::languageForLocale(const QLocale &locale) {
    if (locale.language() == QLocale::Chinese
        || locale.name().startsWith(QStringLiteral("zh"), Qt::CaseInsensitive)) {
        return Language::ChineseSimplified;
    }
    return Language::English;
}

QString LanguageManager::detectLanguageDirectory() const {
    QStringList candidates;
    const QString appDir = QCoreApplication::applicationDirPath();
    candidates << appDir + QStringLiteral("/resource/language");
    candidates << appDir + QStringLiteral("/../resource/language");
    candidates << appDir + QStringLiteral("/../../resource/language");

    for (const QString &path : candidates) {
        QDir dir(path);
        if (dir.exists()) {
            return dir.absolutePath();
        }
    }
    return QString();
}

QString LanguageManager::languageFileName(Language language) const {
    switch (language) {
    case Language::ChineseSimplified:
        return QStringLiteral("zh_CN.ini");
    case Language::English:
        return QStringLiteral("en_US.ini");
    }
    return QStringLiteral("zh_CN.ini");
}

bool LanguageManager::loadFile(const QString &filePath, QHash<int, QString> &buffer) const {
    QFileInfo info(filePath);
    if (!info.exists()) {
        qWarning() << "Language file missing:" << filePath;
        return false;
    }

    QSettings settings(filePath, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");
    const QStringList keys = settings.allKeys();
    buffer.clear();
    for (const QString &rawKey : keys) {
        const QString normalizedKey = rawKey.section(QLatin1Char('/'), -1);
        bool ok = false;
        const int id = normalizedKey.toInt(&ok);
        if (!ok) {
            continue;
        }
        const QString value = settings.value(rawKey).toString();
        if (!value.isEmpty()) {
            buffer.insert(id, value);
        }
    }
    return true;
}

QString LanguageManager::lookupText(int id, const QString &fallback) const {
    QReadLocker locker(&m_lock);
    const auto it = m_translations.constFind(id);
    if (it != m_translations.cend() && !it.value().isEmpty()) {
        return it.value();
    }
    return fallback;
}
