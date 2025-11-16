#pragma once

#include <QHash>
#include <QObject>
#include <QTextObjectInterface>

class QTextDocument;

/*!
 * \brief EmojiImageHandler 使用 QMovie 让 QTextDocument 中的 <img> 支持 GIF 动画
 */
class EmojiImageHandler : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit EmojiImageHandler(QTextDocument *document, QObject *parent = nullptr);

    static void install(QTextDocument *document, QObject *parent = nullptr);

    QSizeF intrinsicSize(QTextDocument *doc, int pos, const QTextFormat &format) override;
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int pos,
                    const QTextFormat &format) override;

private:
    QString resolveSource(const QString &src) const;
    QMovie *movieForSource(const QString &src);

    QTextDocument *m_document = nullptr;
    QHash<QString, QMovie *> m_movies;
};
