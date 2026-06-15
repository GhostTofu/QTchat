#include "friendlistwidget.h"
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QByteArray>
#include <QHeaderView>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QDebug>

FriendListWidget::FriendListWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setIndentation(20);
    setAnimated(true);
    setIconSize(QSize(36, 36));
    setStyleSheet(
        "QTreeWidget { border: none; background: #f5f5f5; }"
        "QTreeWidget::item { padding: 4px; border-bottom: 1px solid #e0e0e0; }"
        "QTreeWidget::item:hover { background: #e3f2fd; }"
        "QTreeWidget::item:selected { background: #bbdefb; }"
    );

    // 预设分组
    ensureGroupItem("家人");
    ensureGroupItem("朋友");
    ensureGroupItem("同事");
    ensureGroupItem("其他");
}

void FriendListWidget::loadFriends(const QVariantList &friends)
{
    clear();
    // 重建分组
    ensureGroupItem("家人");
    ensureGroupItem("朋友");
    ensureGroupItem("同事");
    ensureGroupItem("其他");

    for (const QVariant &v : friends) {
        addFriendToGroup(v.toMap());
    }

    expandAll();
}

void FriendListWidget::addFriendToGroup(const QVariantMap &friendInfo)
{
    QString groupName = friendInfo["groupName"].toString();
    if (groupName.isEmpty()) groupName = "其他";

    QTreeWidgetItem *group = ensureGroupItem(groupName);
    QTreeWidgetItem *item = new QTreeWidgetItem(group);

    QString nickname = friendInfo["nickname"].toString();
    QString account = friendInfo["account"].toString();
    bool online = friendInfo["online"].toBool();
    QString statusIcon = online ? "🟢" : "⚫";

    item->setText(0, QString("%1  %2  [%3]").arg(statusIcon, nickname, account));
    item->setData(0, Qt::UserRole, account);
    item->setData(0, Qt::UserRole + 1, nickname);
    item->setData(0, Qt::UserRole + 2, online);
    item->setData(0, Qt::UserRole + 3, friendInfo["avatar"].toString());
    item->setSizeHint(0, QSize(0, 42));
    applyAvatarToItem(item, friendInfo["avatar"].toString());
    item->setToolTip(0, QString("账号: %1\n昵称: %2\n状态: %3")
                        .arg(account, nickname, online ? "在线" : "离线"));
}

void FriendListWidget::updateFriendStatus(const QString &account, bool online)
{
    QTreeWidgetItem *item = findFriendItem(account);
    if (!item) return;

    item->setData(0, Qt::UserRole + 2, online);
    QString statusIcon = online ? "🟢" : "⚫";
    QString nickname = item->data(0, Qt::UserRole + 1).toString();
    item->setText(0, QString("%1  %2  [%3]").arg(statusIcon, nickname, account));
    applyAvatarToItem(item, item->data(0, Qt::UserRole + 3).toString());
}

void FriendListWidget::updateFriendAvatar(const QString &account, const QString &avatar)
{
    QTreeWidgetItem *item = findFriendItem(account);
    if (!item) return;

    item->setData(0, Qt::UserRole + 3, avatar);
    applyAvatarToItem(item, avatar);
}

void FriendListWidget::addOrUpdateFriend(const QVariantMap &friendInfo)
{
    QString account = friendInfo["account"].toString();
    QTreeWidgetItem *existing = findFriendItem(account);
    if (existing) {
        // 更新已有好友信息
        bool online = friendInfo["online"].toBool();
        QString statusIcon = online ? "🟢" : "⚫";
        existing->setText(0, QString("%1  %2  [%3]")
                             .arg(statusIcon, friendInfo["nickname"].toString(), account));
        existing->setData(0, Qt::UserRole + 1, friendInfo["nickname"].toString());
        existing->setData(0, Qt::UserRole + 2, online);
        existing->setData(0, Qt::UserRole + 3, friendInfo["avatar"].toString());
        existing->setSizeHint(0, QSize(0, 42));
        applyAvatarToItem(existing, friendInfo["avatar"].toString());
    } else {
        addFriendToGroup(friendInfo);
    }
}

void FriendListWidget::applyAvatarToItem(QTreeWidgetItem *item, const QString &avatar)
{
    if (!item) return;

    QPixmap pix;
    if (avatar.startsWith("data:image/")) {
        int comma = avatar.indexOf(',');
        if (comma >= 0) {
            QByteArray imageData = QByteArray::fromBase64(avatar.mid(comma + 1).toLatin1());
            pix.loadFromData(imageData);
        }
    } else if (!avatar.isEmpty() && avatar != "default") {
        pix.load(avatar);
    }

    if (pix.isNull()) {
        item->setIcon(0, QIcon());
        return;
    }

    item->setIcon(0, QIcon(pix.scaled(iconSize(), Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation)));
}

QString FriendListWidget::currentSelectedAccount() const
{
    QTreeWidgetItem *item = currentItem();
    if (item && item->parent()) {  // 确保是子项而非分组
        return item->data(0, Qt::UserRole).toString();
    }
    return QString();
}

QTreeWidgetItem* FriendListWidget::findFriendItem(const QString &account)
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *group = topLevelItem(i);
        for (int j = 0; j < group->childCount(); ++j) {
            QTreeWidgetItem *item = group->child(j);
            if (item->data(0, Qt::UserRole).toString() == account) {
                return item;
            }
        }
    }
    return nullptr;
}

QTreeWidgetItem* FriendListWidget::ensureGroupItem(const QString &groupName)
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        if (topLevelItem(i)->text(0) == groupName) {
            return topLevelItem(i);
        }
    }
    QTreeWidgetItem *group = new QTreeWidgetItem(this);
    group->setText(0, groupName);
    group->setFlags(group->flags() & ~Qt::ItemIsSelectable);
    QFont font = group->font(0);
    font.setBold(true);
    group->setFont(0, font);
    return group;
}

void FriendListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());
    if (item && item->parent()) {
        QString account = item->data(0, Qt::UserRole).toString();
        QString nickname = item->data(0, Qt::UserRole + 1).toString();
        emit friendDoubleClicked(account, nickname);
    }
    QTreeWidget::mouseDoubleClickEvent(event);
}

void FriendListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());
    if (!item || !item->parent()) return;

    QString account = item->data(0, Qt::UserRole).toString();
    QString currentGroup = item->parent()->text(0);

    QMenu menu(this);
    QStringList groups = {"家人", "朋友", "同事", "其他"};
    for (const QString &g : groups) {
        if (g != currentGroup) {
            QAction *action = menu.addAction("移动到 → " + g);
            connect(action, &QAction::triggered, this, [this, account, g]() {
                emit moveToGroupRequested(account, g);
            });
        }
    }

    menu.exec(event->globalPos());
}
