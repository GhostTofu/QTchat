#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QMap>
#include "friendlistwidget.h"
#include "chatdialog.h"
#include "networkclient.h"

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(const UserInfo &userInfo, QWidget *parent = nullptr);
    ~MainWidget();

private slots:
    void onFriendDoubleClicked(const QString &account, const QString &nickname);
    void onNewMessage(const QString &fromAccount, const QString &nickname,
                      const QString &avatar, const QString &content,
                      const QString &msgType, const QString &timestamp);
    void onStatusChanged(const QString &account, bool online);
    void onAvatarSynced(const QString &account, const QString &avatar);
    void onFriendAdded(const QVariantMap &friendInfo);
    void onAddFriendResult(bool success, const QString &reason, const QVariantMap &friendInfo);
    void onFriendListReceived(const QVariantList &friends);

    void onAddFriendClicked();
    void onChangeAvatarClicked();
    void onMessageAck(const QString &toAccount, const QString &status);

private:
    void connectSignals();
    ChatDialog* openChatDialog(const QString &account, const QString &nickname);
    ChatDialog* findChatDialog(const QString &account);

    Ui::MainWidget *ui;
    UserInfo m_myInfo;

    // 聊天窗口管理
    QMap<QString, ChatDialog*> m_chatDialogs;  // account -> dialog
};

#endif // MAINWIDGET_H
