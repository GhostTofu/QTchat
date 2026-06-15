#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientHandler();

    QString getAccount() const { return m_account; }
    void sendJson(const QJsonObject &json);

signals:
    void disconnected(ClientHandler *handler);
    // 需要ChatServer处理的业务
    void loginRequested(ClientHandler *handler, const QString &account, const QString &password);
    void registerRequested(ClientHandler *handler, const QJsonObject &data);
    void sendMessageRequested(const QString &fromAccount, const QString &toAccount,
                              const QString &content, const QString &msgType);
    void avatarUpdateRequested(const QString &account, const QString &avatar);
    void addFriendRequested(ClientHandler *handler, const QString &friendAccount,
                            const QString &groupName);
    void accountBound(ClientHandler *handler, const QString &account);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *m_socket;
    QString m_account;
    QByteArray m_buffer;

    friend class ChatServer; // 允许ChatServer设置账号
};

#endif // CLIENTHANDLER_H
