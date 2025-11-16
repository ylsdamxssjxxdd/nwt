#pragma once

#include <QFrame>
#include <QVector>

class QGridLayout;
class QWidget;

/*!
 * \brief EmotionPicker 表情选择弹窗，负责展示可用表情并输出用户选择结果
 */
class EmotionPicker : public QFrame {
    Q_OBJECT

public:
    explicit EmotionPicker(QWidget *parent = nullptr);

signals:
    /*!
     * \brief emotionSelected 当用户选中某个表情时发出，携带对应的描述文本与资源路径
     */
    void emotionSelected(const QString &tips, const QString &imageResource);

private:
    struct EmotionItem {
        QString tips;
        QString thumbPath;
        QString imagePath;
    };

    void loadEmotions();
    void buildGrid();
    QString iconPathFor(const QString &fileName) const;

    QVector<EmotionItem> m_emotions;
    QGridLayout *m_gridLayout = nullptr;
    QWidget *m_container = nullptr;
};
