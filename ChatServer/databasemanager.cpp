#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

DatabaseManager* DatabaseManager::m_instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager* DatabaseManager::instance()
{
    if (!m_instance) {
        m_instance = new DatabaseManager();
    }
    return m_instance;
}

bool DatabaseManager::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("server.db");

    if (!m_db.open()) {
        qCritical() << "无法打开数据库:" << m_db.lastError().text();
        return false;
    }

    return createTables();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    // 用户表
    bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "  account TEXT PRIMARY KEY,"
        "  nickname TEXT NOT NULL,"
        "  password TEXT NOT NULL,"
        "  gender TEXT DEFAULT '男',"
        "  avatar TEXT DEFAULT 'default',"
        "  online INTEGER DEFAULT 0"
        ")");
    if (!ok) {
        qCritical() << "创建users表失败:" << query.lastError().text();
        return false;
    }

    // 好友关系表
    ok = query.exec(
        "CREATE TABLE IF NOT EXISTS friends ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  account TEXT NOT NULL,"
        "  friendAccount TEXT NOT NULL,"
        "  groupName TEXT DEFAULT '其他',"
        "  UNIQUE(account, friendAccount)"
        ")");
    if (!ok) {
        qCritical() << "创建friends表失败:" << query.lastError().text();
        return false;
    }

    // 消息记录表
    ok = query.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender TEXT NOT NULL,"
        "  receiver TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  msgType TEXT DEFAULT 'text',"
        "  timestamp TEXT NOT NULL"
        ")");
    if (!ok) {
        qCritical() << "创建messages表失败:" << query.lastError().text();
        return false;
    }

    qDebug() << "数据库初始化完成";
    return true;
}

bool DatabaseManager::registerUser(const QString &account, const QString &nickname,
                                    const QString &password, const QString &gender,
                                    const QString &avatar)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (account, nickname, password, gender, avatar) "
                  "VALUES (:acc, :nick, :pwd, :gen, :avt)");
    query.bindValue(":acc", account);
    query.bindValue(":nick", nickname);
    query.bindValue(":pwd", password);
    query.bindValue(":gen", gender);
    query.bindValue(":avt", avatar);

    if (!query.exec()) {
        qWarning() << "注册用户失败:" << query.lastError().text();
        return false;
    }
    return true;
}

QVariantMap DatabaseManager::loginUser(const QString &account, const QString &password)
{
    QVariantMap result;
    result["success"] = false;

    QSqlQuery query(m_db);
    query.prepare("SELECT account, nickname, gender, avatar, online FROM users "
                  "WHERE account = :acc AND password = :pwd");
    query.bindValue(":acc", account);
    query.bindValue(":pwd", password);

    if (query.exec() && query.next()) {
        result["success"] = true;
        result["account"] = query.value("account").toString();
        result["nickname"] = query.value("nickname").toString();
        result["gender"] = query.value("gender").toString();
        result["avatar"] = query.value("avatar").toString();
    }

    return result;
}

bool DatabaseManager::userExists(const QString &account)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM users WHERE account = :acc");
    query.bindValue(":acc", account);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

QVariantMap DatabaseManager::getUserInfo(const QString &account)
{
    QVariantMap info;
    QSqlQuery query(m_db);
    query.prepare("SELECT account, nickname, gender, avatar, online FROM users WHERE account = :acc");
    query.bindValue(":acc", account);

    if (query.exec() && query.next()) {
        info["account"] = query.value("account").toString();
        info["nickname"] = query.value("nickname").toString();
        info["gender"] = query.value("gender").toString();
        info["avatar"] = query.value("avatar").toString();
        info["online"] = query.value("online").toBool();
    }
    return info;
}

bool DatabaseManager::updateUserAvatar(const QString &account, const QString &avatar)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET avatar = :avt WHERE account = :acc");
    query.bindValue(":avt", avatar);
    query.bindValue(":acc", account);
    return query.exec();
}

bool DatabaseManager::setUserOnline(const QString &account, bool online)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET online = :on WHERE account = :acc");
    query.bindValue(":on", online ? 1 : 0);
    query.bindValue(":acc", account);
    return query.exec();
}

QStringList DatabaseManager::getAllAccounts()
{
    QStringList accounts;
    QSqlQuery query(m_db);
    query.exec("SELECT account FROM users");
    while (query.next()) {
        accounts << query.value(0).toString();
    }
    return accounts;
}

bool DatabaseManager::addFriend(const QString &account, const QString &friendAccount,
                                 const QString &groupName)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO friends (account, friendAccount, groupName) "
                  "VALUES (:acc, :frd, :grp)");
    query.bindValue(":acc", account);
    query.bindValue(":frd", friendAccount);
    query.bindValue(":grp", groupName);
    return query.exec();
}

bool DatabaseManager::removeFriend(const QString &account, const QString &friendAccount)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM friends WHERE account = :acc AND friendAccount = :frd");
    query.bindValue(":acc", account);
    query.bindValue(":frd", friendAccount);
    return query.exec();
}

QVariantList DatabaseManager::getFriendList(const QString &account)
{
    QVariantList friends;
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT f.friendAccount, f.groupName, u.nickname, u.gender, u.avatar, u.online "
        "FROM friends f "
        "JOIN users u ON f.friendAccount = u.account "
        "WHERE f.account = :acc "
        "ORDER BY f.groupName, f.friendAccount");
    query.bindValue(":acc", account);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap frd;
            frd["account"] = query.value("friendAccount").toString();
            frd["groupName"] = query.value("groupName").toString();
            frd["nickname"] = query.value("nickname").toString();
            frd["gender"] = query.value("gender").toString();
            frd["avatar"] = query.value("avatar").toString();
            frd["online"] = query.value("online").toBool();
            friends.append(frd);
        }
    }
    return friends;
}

bool DatabaseManager::updateFriendGroup(const QString &account, const QString &friendAccount,
                                         const QString &groupName)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE friends SET groupName = :grp "
                  "WHERE account = :acc AND friendAccount = :frd");
    query.bindValue(":grp", groupName);
    query.bindValue(":acc", account);
    query.bindValue(":frd", friendAccount);
    return query.exec();
}

bool DatabaseManager::saveMessage(const QString &sender, const QString &receiver,
                                   const QString &content, const QString &msgType)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (sender, receiver, content, msgType, timestamp) "
                  "VALUES (:snd, :rcv, :cnt, :typ, :ts)");
    query.bindValue(":snd", sender);
    query.bindValue(":rcv", receiver);
    query.bindValue(":cnt", content);
    query.bindValue(":typ", msgType);
    query.bindValue(":ts", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    return query.exec();
}

QVariantList DatabaseManager::getChatHistory(const QString &user1, const QString &user2, int limit)
{
    QVariantList history;
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT sender, receiver, content, msgType, timestamp FROM messages "
        "WHERE (sender = :u1 AND receiver = :u2) OR (sender = :u2 AND receiver = :u1) "
        "ORDER BY id ASC LIMIT :lim");
    query.bindValue(":u1", user1);
    query.bindValue(":u2", user2);
    query.bindValue(":lim", limit);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap msg;
            msg["sender"] = query.value("sender").toString();
            msg["receiver"] = query.value("receiver").toString();
            msg["content"] = query.value("content").toString();
            msg["msgType"] = query.value("msgType").toString();
            msg["timestamp"] = query.value("timestamp").toString();
            history.append(msg);
        }
    }
    return history;
}
