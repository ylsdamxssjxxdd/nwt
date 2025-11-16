#include "EmotionPicker.h"

#include <QFile>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSize>
#include <QToolButton>
#include <QVBoxLayout>
#include <QXmlStreamReader>

namespace {
constexpr int kEmotionColumns = 8;
constexpr int kIconSize = 32;
constexpr int kButtonPadding = 12;
const char *kEmotionResourcePrefix = ":/ui/emotion/";
}

EmotionPicker::EmotionPicker(QWidget *parent) : QFrame(parent) {
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setObjectName("emotionPopup");
    setFixedSize(380, 260);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_container = new QWidget(scrollArea);
    m_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_gridLayout = new QGridLayout(m_container);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setHorizontalSpacing(6);
    m_gridLayout->setVerticalSpacing(6);

    scrollArea->setWidget(m_container);
    layout->addWidget(scrollArea);

    loadEmotions();
    buildGrid();
}

void EmotionPicker::loadEmotions() {
    QFile file(QStringLiteral(":/ui/emotion/Emotion.xml"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        const auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement && reader.name() == QLatin1String("Emotion")) {
            const auto attrs = reader.attributes();
            EmotionItem item;
            item.tips = attrs.value(QStringLiteral("Tips")).toString();
            item.thumbPath = attrs.value(QStringLiteral("Thumb")).toString();
            item.imagePath = attrs.value(QStringLiteral("Image")).toString();
            if (!resourceExists(item.imagePath)) {
                item.imagePath = item.thumbPath;
            }
            if (item.imagePath.isEmpty()) {
                item.imagePath = item.thumbPath;
            }
            if (!item.tips.isEmpty() && resourceExists(item.thumbPath) && resourceExists(item.imagePath)) {
                m_emotions.append(item);
            }
        }
    }
}

void EmotionPicker::buildGrid() {
    if (!m_gridLayout) {
        return;
    }
    int row = 0;
    int column = 0;
    int added = 0;
    const int buttonSide = kIconSize + kButtonPadding;
    for (const auto &emotion : m_emotions) {
        if (!resourceExists(emotion.thumbPath)) {
            continue;
        }
        const QPixmap pix(iconPathFor(emotion.thumbPath));
        auto *button = new QToolButton(m_container);
        button->setAutoRaise(true);
        button->setIcon(QIcon(pix));
        button->setIconSize(QSize(kIconSize, kIconSize));
        button->setFixedSize(buttonSide, buttonSide);
        button->setToolTip(emotion.tips);
        connect(button, &QToolButton::clicked, this, [this, emotion]() {
            emit emotionSelected(emotion.tips, iconPathFor(emotion.imagePath));
            hide();
        });
        m_gridLayout->addWidget(button, row, column);
        ++column;
        ++added;
        if (column >= kEmotionColumns) {
            column = 0;
            ++row;
        }
    }
    if (added == 0) {
        auto *hint = new QLabel(tr(u8"未找到可用的表情资源"), m_container);
        hint->setAlignment(Qt::AlignCenter);
        m_gridLayout->addWidget(hint, 0, 0);
    }
    m_gridLayout->setRowStretch(row + 1, 1);
}

QString EmotionPicker::iconPathFor(const QString &fileName) const {
    return QString::fromUtf8(kEmotionResourcePrefix) + fileName;
}

bool EmotionPicker::resourceExists(const QString &fileName) const {
    QFile res(iconPathFor(fileName));
    return res.exists();
}
