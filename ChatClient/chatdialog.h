#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include <QEvent>
#include "emojipicker.h"

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChatDialog(const QString &myAccount, const QString &friendAccount,
                        const QString &friendNickname, QWidget *parent = nullptr);
    ~ChatDialog();

    QString friendAccount() const { return m_friendAccount; }

    void appendMessage(const QString &sender, const QString &senderName,
                       const QString &content, const QString &timestamp,
                       bool isMine);
    void loadHistory(const QVariantList &messages);

public slots:
    void onRecvMessage(const QString &content, const QString &timestamp);

signals:
    void sendMessage(const QString &toAccount, const QString &content, const QString &msgType);

private slots:
    void onSendClicked();
    void onEmojiClicked();
    void onExportHistory();
    void onEmojiSelected(const QString &emoji);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::ChatDialog *ui;

    QString m_myAccount;
    QString m_friendAccount;
    QString m_friendNickname;

    EmojiPicker *m_emojiPicker;
};

#endif // CHATDIALOG_H
