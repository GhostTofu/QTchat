#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>

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

bool DatabaseManager::initDatabase(const QString &myAccount)
{
    m_myAccount = myAccount;
    QString dbName = QString("chat_%1.db").arg(myAccount);

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qCritical() << "客户端数据库打开失败:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender TEXT NOT NULL,"
        "  receiver TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  msgType TEXT DEFAULT 'text',"
        "  timestamp TEXT NOT NULL"
        ")");

    if (!ok) {
        qCritical() << "创建消息表失败:" << query.lastError().text();
        return false;
    }

    qDebug() << "客户端数据库初始化完成:" << dbName;
    return true;
}

bool DatabaseManager::saveMessage(const QString &sender, const QString &receiver,
                                   const QString &content, const QString &msgType,
                                   const QString &timestamp)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (sender, receiver, content, msgType, timestamp) "
                  "VALUES (:snd, :rcv, :cnt, :typ, :ts)");
    query.bindValue(":snd", sender);
    query.bindValue(":rcv", receiver);
    query.bindValue(":cnt", content);
    query.bindValue(":typ", msgType);
    query.bindValue(":ts", timestamp);
    return query.exec();
}

QVariantList DatabaseManager::getChatHistory(const QString &otherAccount, int limit)
{
    QVariantList history;
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT sender, receiver, content, msgType, timestamp FROM messages "
        "WHERE (sender = :me AND receiver = :other) "
        "   OR (sender = :other AND receiver = :me) "
        "ORDER BY id ASC LIMIT :lim");
    query.bindValue(":me", m_myAccount);
    query.bindValue(":other", otherAccount);
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

QString DatabaseManager::exportChatHistory(const QString &otherAccount)
{
    QVariantList history = getChatHistory(otherAccount, 9999);
    if (history.isEmpty()) {
        return QString();
    }

    QString fileName = QString("chat_history_%1_%2.txt").arg(m_myAccount, otherAccount);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QString("===== 聊天记录: %1 <-> %2 =====\n\n").arg(m_myAccount, otherAccount);

    for (const QVariant &v : history) {
        QVariantMap msg = v.toMap();
        out << QString("[%1] %2: %3\n")
               .arg(msg["timestamp"].toString(),
                    msg["sender"].toString(),
                    msg["content"].toString());
    }

    file.close();
    return QDir::currentPath() + "/" + fileName;
}
