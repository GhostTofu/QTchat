#ifndef FRIENDLISTWIDGET_H
#define FRIENDLISTWIDGET_H

#include <QTreeWidget>
#include <QVariantMap>

class FriendListWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit FriendListWidget(QWidget *parent = nullptr);

    void loadFriends(const QVariantList &friends);
    void updateFriendStatus(const QString &account, bool online);
    void updateFriendAvatar(const QString &account, const QString &avatar);
    void addOrUpdateFriend(const QVariantMap &friendInfo);
    QString currentSelectedAccount() const;

signals:
    void friendDoubleClicked(const QString &account, const QString &nickname);
    void moveToGroupRequested(const QString &account, const QString &newGroup);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QTreeWidgetItem* findFriendItem(const QString &account);
    QTreeWidgetItem* ensureGroupItem(const QString &groupName);
    void addFriendToGroup(const QVariantMap &friendInfo);
    void applyAvatarToItem(QTreeWidgetItem *item, const QString &avatar);
};

#endif // FRIENDLISTWIDGET_H
