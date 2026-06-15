#include "emojipicker.h"
#include "ui_emojipicker.h"
#include <QGridLayout>
#include <QPushButton>

EmojiPicker::EmojiPicker(QWidget *parent)
    : QWidget(parent, Qt::Popup), ui(new Ui::EmojiPicker)
{
    ui->setupUi(this);
    setupEmojiGrid();
}

EmojiPicker::~EmojiPicker()
{
    delete ui;
}

void EmojiPicker::setupEmojiGrid()
{
    // 常用emoji列表
    m_emojis = QStringList()
        << "😀" << "😂" << "🤣" << "😊" << "😍" << "🤩"
        << "😘" << "😜" << "🤔" << "😎" << "😢" << "😡"
        << "👍" << "👎" << "👏" << "🙏" << "💪" << "🤝"
        << "❤️" << "💔" << "🔥" << "⭐" << "🎉" << "🎂"
        << "🌹" << "☕" << "🍺" << "🏠" << "✈️" << "🚗"
        << "⏰" << "💰" << "📱" << "💻" << "📷" << "🎵";

    QGridLayout *grid = ui->gridLayout;
    for (int i = 0; i < m_emojis.size(); ++i) {
        QPushButton *btn = new QPushButton(m_emojis[i], this);
        btn->setFixedSize(36, 36);
        btn->setStyleSheet("QPushButton { font-size:18px; border:none; background:transparent; }"
                           "QPushButton:hover { background-color:#e0e0e0; border-radius:4px; }");
        btn->setToolTip(m_emojis[i]);
        connect(btn, &QPushButton::clicked, this, &EmojiPicker::onEmojiClicked);
        grid->addWidget(btn, i / 6, i % 6);
    }
}

void EmojiPicker::onEmojiClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        emit emojiSelected(btn->text());
    }
}
