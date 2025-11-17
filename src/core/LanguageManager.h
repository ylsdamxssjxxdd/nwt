#pragma once

#include <QObject>
#include <QHash>
#include <QLocale>
#include <QReadWriteLock>
#include <QString>

/**
 * @brief INI 语言文件管理器。
 *
 * 负责从 resource/language 目录读取以「数字编号=译文」形式保存的翻译，
 * 并在运行时按编号返回对应文本，未命中时会回落到调用方给定的默认文案。
 */
class LanguageManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 支持的语言集合。
     */
    enum class Language {
        ChineseSimplified,
        English
    };

    /**
     * @brief 获取全局语言管理器实例。
     */
    static LanguageManager &instance();

    /**
     * @brief 快速查询编号对应的文本。
     * @param id 数字编号。
     * @param fallback 翻译缺失时的回退文本。
     */
    static QString text(int id, const QString &fallback = QString());

    /**
     * @brief 初始化语言目录。
     * @param customLanguageDir 若为空则自动在应用目录附近查找 resource/language。
     */
    bool initialize(const QString &customLanguageDir = QString());

    /**
     * @brief 加载并切换到指定语言。
     */
    bool switchLanguage(Language language);

    /**
     * @brief 当前语言。
     */
    Language currentLanguage() const;

    /**
     * @brief 当前语言目录的绝对路径。
     */
    QString languageDirectory() const;

    /**
     * @brief 返回语言对应的标准编码（不含扩展名）。
     */
    static QString codeFor(Language language);

    /**
     * @brief 由编码解析语言。
     */
    static Language languageFromCode(const QString &code);

    /**
     * @brief 根据系统区域语言推断默认语言。
     */
    static Language languageForLocale(const QLocale &locale);

signals:
    /**
     * @brief 当语言切换完成后触发。
     */
    void languageChanged(Language language);

private:
    explicit LanguageManager(QObject *parent = nullptr);
    QString detectLanguageDirectory() const;
    QString languageFileName(Language language) const;
    bool loadFile(const QString &filePath, QHash<int, QString> &buffer) const;
    QString lookupText(int id, const QString &fallback) const;

    mutable QReadWriteLock m_lock;
    QHash<int, QString> m_translations;
    QString m_languageDir;
    Language m_currentLanguage = Language::ChineseSimplified;
};
