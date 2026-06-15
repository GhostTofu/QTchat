#ifndef EMOJIPICKER_H
#define EMOJIPICKER_H

#include <QWidget>

namespace Ui {
class EmojiPicker;
}

class EmojiPicker : public QWidget
{
    Q_OBJECT
public:
    explicit EmojiPicker(QWidget *parent = nullptr);
    ~EmojiPicker();

signals:
    void emojiSelected(const QString &emoji);

private slots:
    void onEmojiClicked();

private:
    void setupEmojiGrid();
    Ui::EmojiPicker *ui;
    QStringList m_emojis;
};

#endif // EMOJIPICKER_H
