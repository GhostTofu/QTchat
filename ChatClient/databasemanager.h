#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager* instance();

    bool initDatabase(const QString &myAccount);

    // 聊天记录
    bool saveMessage(const QString &sender, const QString &receiver,
                     const QString &content, const QString &msgType,
                     const QString &timestamp);
    QVariantList getChatHistory(const QString &otherAccount, int limit = 100);

    // 导出聊天记录
    QString exportChatHistory(const QString &otherAccount);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    static DatabaseManager *m_instance;
    QSqlDatabase m_db;
    QString m_myAccount;
};

#endif // DATABASEMANAGER_H
