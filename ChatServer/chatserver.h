#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QJsonObject>

class ClientHandler;

class ChatServer : public QObject
{
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);
    ~ChatServer();

    bool startServer(int port = 8888);
    void stopServer();
    bool isRunning() const;
    int clientCount() const;

signals:
    // GUI 更新信号
    void serverStarted(int port);
    void serverStopped();
    void logMessage(const QString &message);
    void clientConnected(const QString &address, int port);
    void clientDisconnected(const QString &account);
    void clientLoggedIn(const QString &account, const QString &nickname);
    void messageRelayed(const QString &from, const QString &to,
                        const QString &content, const QString &msgType);
    void errorOccurred(const QString &error);

private slots:
    void onNewConnection();
    void onClientDisconnected(ClientHandler *handler);

    // 处理客户端请求
    void onLoginRequested(ClientHandler *handler, const QString &account,
                          const QString &password);
    void onRegisterRequested(ClientHandler *handler, const QJsonObject &data);
    void onSendMessage(const QString &fromAccount, const QString &toAccount,
                       const QString &content, const QString &msgType);
    void onAvatarUpdate(const QString &account, const QString &avatar);
    void onAddFriend(ClientHandler *handler, const QString &friendAccount,
                     const QString &groupName);

private:
    QTcpServer *m_server;
    QMap<QString, ClientHandler*> m_clients;  // account -> handler
    QList<ClientHandler*> m_handlers;          // 所有连接

    ClientHandler* findHandler(const QString &account);
    void broadcastStatusToFriends(const QString &account, bool online);
    void sendFriendList(ClientHandler *handler, const QString &account);
};

#endif // CHATSERVER_H
