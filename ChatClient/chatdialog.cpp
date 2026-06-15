#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QKeyEvent>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableFormat>
#include <QDebug>

ChatDialog::ChatDialog(const QString &myAccount, const QString &friendAccount,
                         const QString &friendNickname, QWidget *parent)
    : QDialog(parent), ui(new Ui::ChatDialog),
      m_myAccount(myAccount), m_friendAccount(friendAccount),
      m_friendNickname(friendNickname), m_emojiPicker(nullptr)
{
    ui->setupUi(this);
    setWindowTitle(QString("与 %1 聊天中").arg(friendNickname));
    resize(500, 550);
    ui->chatView->setLayoutDirection(Qt::LeftToRight);
    ui->chatView->document()->setDefaultStyleSheet(
        "table.message-row { width: 100%; }"
        ".bubble-mine { background:#2196f3; color:white; padding:6px 10px; border-radius:8px; }"
        ".bubble-other { background:white; color:#222; padding:6px 10px; border-radius:8px; }"
        ".time { font-size:10px; color:#888; }"
        ".name { font-size:11px; font-weight:bold; color:#333; }"
    );

    // 连接信号
    connect(ui->sendBtn, &QPushButton::clicked, this, &ChatDialog::onSendClicked);
    connect(ui->emojiBtn, &QPushButton::clicked, this, &ChatDialog::onEmojiClicked);
    connect(ui->exportBtn, &QPushButton::clicked, this, &ChatDialog::onExportHistory);
    ui->inputEdit->installEventFilter(this);

    // 加载历史记录
    QVariantList history = DatabaseManager::instance()->getChatHistory(friendAccount);
    if (!history.isEmpty()) {
        loadHistory(history);
    }
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::appendMessage(const QString &sender, const QString &senderName,
                                const QString &content, const QString &timestamp,
                                bool isMine)
{
    Q_UNUSED(sender);

    QString html;
    if (isMine) {
        // 自己的消息：右对齐，蓝色背景
        html = QString(
            "<div align='right'>"
            "<span class='time'>%1 </span>"
            "<span class='bubble-mine'>%2</span>"
            "</div>")
            .arg(timestamp, content.toHtmlEscaped());
    } else {
        // 对方消息：左对齐，白色背景
        html = QString(
            "<div align='left'>"
            "<span class='name'>%1</span><br>"
            "<span class='bubble-other'>%2</span>"
            "<span class='time'> %3</span>"
            "</div>")
            .arg(senderName.toHtmlEscaped(), content.toHtmlEscaped(), timestamp);
    }

    QTextCursor cursor = ui->chatView->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (!ui->chatView->document()->isEmpty()) {
        cursor.insertBlock();
    }

    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setCellPadding(0);
    tableFormat.setCellSpacing(0);
    tableFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    tableFormat.setColumnWidthConstraints({
        QTextLength(QTextLength::PercentageLength, 50),
        QTextLength(QTextLength::PercentageLength, 50)
    });

    QTextTable *table = cursor.insertTable(1, 2, tableFormat);
    QTextCursor cellCursor = table->cellAt(0, isMine ? 1 : 0).firstCursorPosition();
    QTextBlockFormat cellBlockFormat;
    cellBlockFormat.setAlignment(isMine ? Qt::AlignRight : Qt::AlignLeft);
    cellCursor.setBlockFormat(cellBlockFormat);
    cellCursor.insertHtml(html);

    cursor = table->lastCursorPosition();
    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();

    ui->chatView->setTextCursor(cursor);
    QScrollBar *sb = ui->chatView->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void ChatDialog::loadHistory(const QVariantList &messages)
{
    for (const QVariant &v : messages) {
        QVariantMap msg = v.toMap();
        QString sender = msg["sender"].toString();
        bool isMine = (sender == m_myAccount);
        QString senderName = isMine ? "我" : m_friendNickname;

        appendMessage(sender, senderName,
                      msg["content"].toString(),
                      msg["timestamp"].toString(),
                      isMine);
    }
}

void ChatDialog::onRecvMessage(const QString &content, const QString &timestamp)
{
    appendMessage(m_friendAccount, m_friendNickname, content, timestamp, false);
}

void ChatDialog::onSendClicked()
{
    QString content = ui->inputEdit->toPlainText().trimmed();
    if (content.isEmpty()) return;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

    // 显示自己的消息
    appendMessage(m_myAccount, "我", content, timestamp, true);

    // 保存到本地数据库
    DatabaseManager::instance()->saveMessage(m_myAccount, m_friendAccount,
                                              content, "text", timestamp);

    // 发送到服务器
    emit sendMessage(m_friendAccount, content, "text");

    ui->inputEdit->clear();
    ui->inputEdit->setFocus();
}

void ChatDialog::onEmojiClicked()
{
    if (!m_emojiPicker) {
        m_emojiPicker = new EmojiPicker(this);
        connect(m_emojiPicker, &EmojiPicker::emojiSelected,
                this, &ChatDialog::onEmojiSelected);
    }

    // 在表情按钮下方弹出
    QPoint pos = ui->emojiBtn->mapToGlobal(QPoint(0, ui->emojiBtn->height()));
    m_emojiPicker->move(pos);
    m_emojiPicker->show();
}

void ChatDialog::onEmojiSelected(const QString &emoji)
{
    ui->inputEdit->insertPlainText(emoji);
    m_emojiPicker->hide();
    ui->inputEdit->setFocus();
}

void ChatDialog::onExportHistory()
{
    QString filePath = DatabaseManager::instance()->exportChatHistory(m_friendAccount);
    if (filePath.isEmpty()) {
        QMessageBox::information(this, "导出", "暂无聊天记录可导出。");
    } else {
        QMessageBox::information(this, "导出成功",
                                 QString("聊天记录已导出到:\n%1").arg(filePath));
    }
}

// EventFilter处理Enter发送
bool ChatDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->inputEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ControlModifier) {
                return QDialog::eventFilter(obj, event);
            } else {
                onSendClicked();
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}
