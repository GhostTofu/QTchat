#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantMap>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager* instance();

    bool initDatabase();

    // 用户操作
    bool registerUser(const QString &account, const QString &nickname,
                      const QString &password, const QString &gender,
                      const QString &avatar);
    QVariantMap loginUser(const QString &account, const QString &password);
    bool userExists(const QString &account);
    QVariantMap getUserInfo(const QString &account);
    bool updateUserAvatar(const QString &account, const QString &avatar);
    bool setUserOnline(const QString &account, bool online);
    QStringList getAllAccounts();

    // 好友操作
    bool addFriend(const QString &account, const QString &friendAccount,
                   const QString &groupName = "其他");
    bool removeFriend(const QString &account, const QString &friendAccount);
    QVariantList getFriendList(const QString &account);
    bool updateFriendGroup(const QString &account, const QString &friendAccount,
                           const QString &groupName);

    // 消息操作
    bool saveMessage(const QString &sender, const QString &receiver,
                     const QString &content, const QString &msgType);
    QVariantList getChatHistory(const QString &user1, const QString &user2, int limit = 50);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    static DatabaseManager *m_instance;
    QSqlDatabase m_db;

    bool createTables();
};

#endif // DATABASEMANAGER_H
