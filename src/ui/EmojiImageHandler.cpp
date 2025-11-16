#include "EmojiImageHandler.h"

#include <QAbstractTextDocumentLayout>
#include <QMovie>
#include <QPainter>
#include <QTextDocument>

EmojiImageHandler::EmojiImageHandler(QTextDocument *document, QObject *parent)
    : QObject(parent), m_document(document) {}

void EmojiImageHandler::install(QTextDocument *document, QObject *parent) {
    if (!document || !document->documentLayout()) {
        return;
    }
    auto *handler = new EmojiImageHandler(document, parent);
    document->documentLayout()->registerHandler(QTextFormat::ImageObject, handler);
}

QString EmojiImageHandler::resolveSource(const QString &src) const {
    if (src.startsWith(QStringLiteral("qrc:/"))) {
        return QStringLiteral(":/") + src.mid(5);
    }
    return src;
}

QMovie *EmojiImageHandler::movieForSource(const QString &src) {
    const QString path = resolveSource(src);
    auto it = m_movies.find(path);
    if (it != m_movies.end()) {
        return it.value();
    }
    auto *movie = new QMovie(path, QByteArray(), this);
    if (!movie->isValid()) {
        delete movie;
        movie = nullptr;
    } else {
        movie->setCacheMode(QMovie::CacheAll);
        movie->start();
        connect(movie, &QMovie::frameChanged, this, [this]() {
            if (m_document && m_document->documentLayout()) {
                m_document->documentLayout()->update();
            }
        });
    }
    m_movies.insert(path, movie);
    return movie;
}

QSizeF EmojiImageHandler::intrinsicSize(QTextDocument *, int, const QTextFormat &format) {
    const auto imageFormat = format.toImageFormat();
    const QString name = imageFormat.name();
    if (auto *movie = movieForSource(name)) {
        const QSize size = movie->currentPixmap().size();
        if (!size.isEmpty()) {
            return size;
        }
    }
    if (imageFormat.width() > 0 && imageFormat.height() > 0) {
        return QSizeF(imageFormat.width(), imageFormat.height());
    }
    return QSizeF(24, 24);
}

void EmojiImageHandler::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *, int,
                                   const QTextFormat &format) {
    const auto imageFormat = format.toImageFormat();
    const QString name = imageFormat.name();
    if (auto *movie = movieForSource(name)) {
        painter->drawPixmap(rect.topLeft(), movie->currentPixmap().scaled(
                                                rect.size().toSize(),
                                                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }
    const QPixmap pix(resolveSource(name));
    if (!pix.isNull()) {
        painter->drawPixmap(rect.topLeft(),
                            pix.scaled(rect.size().toSize(),
                                       Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
