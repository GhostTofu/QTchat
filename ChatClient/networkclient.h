#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QVariantMap>

// 用户信息结构
struct UserInfo {
    QString account;
    QString nickname;
    QString gender;
    QString avatar;
};

class NetworkClient : public QObject
{
    Q_OBJECT
public:
    static NetworkClient* instance();

    void connectToServer(const QString &host, int port = 8888);
    void disconnectFromServer();
    bool isConnected() const;

    // 发送请求
    void sendLogin(const QString &account, const QString &password);
    void sendRegister(const UserInfo &user, const QString &password);
    void sendMessage(const QString &toAccount, const QString &content,
                     const QString &msgType = "text");
    void sendAvatarUpdate(const QString &account, const QString &avatar);
    void sendAddFriend(const QString &friendAccount, const QString &groupName = "其他");

signals:
    void connected();
    void disconnected();
    void loginResult(bool success, const QString &reason,
                     const UserInfo &userInfo);
    void registerResult(bool success, const QString &reason);
    void friendListReceived(const QVariantList &friends);
    void newMessage(const QString &fromAccount, const QString &nickname,
                    const QString &avatar, const QString &content,
                    const QString &msgType, const QString &timestamp);
    void statusChanged(const QString &account, bool online);
    void avatarSynced(const QString &account, const QString &avatar);
    void friendAdded(const QVariantMap &friendInfo);
    void addFriendResult(bool success, const QString &reason,
                         const QVariantMap &friendInfo);
    void messageAck(const QString &toAccount, const QString &status);

private slots:
    void onReadyRead();
    void onSocketConnected();
    void onSocketDisconnected();

private:
    explicit NetworkClient(QObject *parent = nullptr);
    static NetworkClient *m_instance;

    void sendJson(const QJsonObject &json);
    void processMessage(const QJsonObject &json);

    QTcpSocket *m_socket;
    QByteArray m_buffer;
    QString m_host;
    int m_port;
};

#endif // NETWORKCLIENT_H
