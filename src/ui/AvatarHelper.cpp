#include "AvatarHelper.h"

#include "core/SettingsTypes.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QtGlobal>

namespace {
enum class GenderTone { Male, Female, Neutral };

GenderTone detectGenderTone(const QString &gender) {
    const QString normalized = gender.trimmed().toLower();
    if (normalized.isEmpty()) {
        return GenderTone::Neutral;
    }
    if (normalized.contains(QStringLiteral("女")) || normalized.contains(QStringLiteral("female"))) {
        return GenderTone::Female;
    }
    if (normalized.contains(QStringLiteral("男")) || normalized.contains(QStringLiteral("male"))) {
        return GenderTone::Male;
    }
    return GenderTone::Neutral;
}

QColor toneColor(GenderTone tone) {
    switch (tone) {
    case GenderTone::Female:
        return QColor(QStringLiteral("#ff8ec5"));
    case GenderTone::Male:
        return QColor(QStringLiteral("#4a90e2"));
    case GenderTone::Neutral:
    default:
        return QColor(QStringLiteral("#4a90e2"));
    }
}

QString normalizeInitial(const QString &name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("E");
    }
    const QString first = trimmed.left(1).toUpper();
    return first.isEmpty() ? QStringLiteral("E") : first;
}

QColor accentShadowColor() {
    return QColor(8, 19, 54, 35);
}

QPainterPath roundedPath(const QRectF &rect, qreal radius) {
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

QPixmap renderImageAvatar(const QString &path, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QImage image(path);
    if (image.isNull()) {
        return pixmap;
    }
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal inset = size * 0.08;
    const QRectF frame = QRectF(inset, inset, size - inset * 2, size - inset * 2);
    const qreal radius = frame.width() * 0.28;

    painter.fillPath(roundedPath(frame.translated(0, 2), radius), accentShadowColor());

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::NoBrush);
    const QPainterPath clipPath = roundedPath(frame, radius);
    painter.setClipPath(clipPath);
    const QImage scaled =
        image.scaled(frame.size().toSize(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    painter.drawImage(frame, scaled);
    painter.setClipping(false);

    QPen pen(QColor(255, 255, 255, 220));
    pen.setWidthF(2.0);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(frame, radius, radius);
    return pixmap;
}
} // namespace

namespace AvatarHelper {
AvatarDescriptor descriptorFromProfile(const ProfileDetails &profile) {
    AvatarDescriptor descriptor;
    descriptor.displayName = profile.name;
    descriptor.gender = profile.gender;
    descriptor.avatarPath = profile.avatarPath;
    return descriptor;
}

QPixmap createAvatarPixmap(const AvatarDescriptor &descriptor, int size) {
    const int finalSize = qMax(16, size);
    if (!descriptor.avatarPath.isEmpty()) {
        QPixmap pixmap = renderImageAvatar(descriptor.avatarPath, finalSize);
        if (!pixmap.isNull()) {
            return pixmap;
        }
    }

    QPixmap pixmap(finalSize, finalSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const qreal inset = finalSize * 0.08;
    const QRectF frame = QRectF(inset, inset, finalSize - inset * 2, finalSize - inset * 2);
    const qreal radius = frame.width() * 0.28;
    const QColor background = toneColor(detectGenderTone(descriptor.gender));

    painter.fillPath(roundedPath(frame.translated(0, 2), radius), accentShadowColor());
    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawRoundedRect(frame, radius, radius);

    painter.setPen(QColor(255, 255, 255, 245));
    QFont font(QStringLiteral("Microsoft YaHei"), finalSize / 2.4);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(frame, Qt::AlignCenter, normalizeInitial(descriptor.displayName));

    QPen pen(QColor(255, 255, 255, 220));
    pen.setWidthF(2.0);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(frame, radius, radius);
    return pixmap;
}
} // namespace AvatarHelper
