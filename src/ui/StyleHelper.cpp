#include "StyleHelper.h"

#include <QFile>

namespace UiUtils {
QString loadStyleSheet(const QString &resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}
} // namespace UiUtils
