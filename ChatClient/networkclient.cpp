#include "networkclient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

NetworkClient* NetworkClient::m_instance = nullptr;

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_port(8888)
{
    connect(m_socket, &QTcpSocket::connected,
            this, &NetworkClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &NetworkClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &NetworkClient::onReadyRead);
}

NetworkClient* NetworkClient::instance()
{
    if (!m_instance) {
        m_instance = new NetworkClient();
    }
    return m_instance;
}

void NetworkClient::connectToServer(const QString &host, int port)
{
    m_host = host;
    m_port = port;
    m_socket->connectToHost(host, port);
}

void NetworkClient::disconnectFromServer()
{
    m_socket->disconnectFromHost();
}

bool NetworkClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::onSocketConnected()
{
    qDebug() << "已连接到服务器:" << m_host << ":" << m_port;
    emit connected();
}

void NetworkClient::onSocketDisconnected()
{
    qDebug() << "与服务器断开连接";
    emit disconnected();
}

void NetworkClient::sendJson(const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(data);
    m_socket->flush();
}

// ==================== 发送请求 ====================

void NetworkClient::sendLogin(const QString &account, const QString &password)
{
    QJsonObject json;
    json["type"] = "login";
    json["account"] = account;
    json["password"] = password;
    sendJson(json);
}

void NetworkClient::sendRegister(const UserInfo &user, const QString &password)
{
    QJsonObject json;
    json["type"] = "register";
    json["account"] = user.account;
    json["nickname"] = user.nickname;
    json["password"] = password;
    json["gender"] = user.gender;
    json["avatar"] = user.avatar;
    sendJson(json);
}

void NetworkClient::sendMessage(const QString &toAccount, const QString &content,
                                 const QString &msgType)
{
    QJsonObject json;
    json["type"] = "send_msg";
    json["toAccount"] = toAccount;
    json["content"] = content;
    json["msgType"] = msgType;
    sendJson(json);
}

void NetworkClient::sendAvatarUpdate(const QString &account, const QString &avatar)
{
    QJsonObject json;
    json["type"] = "avatar_update";
    json["account"] = account;
    json["avatar"] = avatar;
    sendJson(json);
}

void NetworkClient::sendAddFriend(const QString &friendAccount, const QString &groupName)
{
    QJsonObject json;
    json["type"] = "add_friend";
    json["friendAccount"] = friendAccount;
    json["groupName"] = groupName;
    sendJson(json);
}

// ==================== 接收处理 ====================

void NetworkClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (m_buffer.contains('\n')) {
        int idx = m_buffer.indexOf('\n');
        QByteArray line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);

        if (line.isEmpty()) continue;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "客户端JSON解析失败:" << error.errorString();
            continue;
        }

        processMessage(doc.object());
    }
}

void NetworkClient::processMessage(const QJsonObject &json)
{
    QString type = json["type"].toString();

    if (type == "login_resp") {
        UserInfo info;
        info.account = json["account"].toString();
        info.nickname = json["nickname"].toString();
        info.gender = json["gender"].toString();
        info.avatar = json["avatar"].toString();
        emit loginResult(json["success"].toBool(),
                         json["reason"].toString(),
                         info);

    } else if (type == "register_resp") {
        emit registerResult(json["success"].toBool(),
                            json["reason"].toString());

    } else if (type == "friend_list") {
        QVariantList friends;
        QJsonArray arr = json["friends"].toArray();
        for (const QJsonValue &v : arr) {
            friends.append(v.toObject().toVariantMap());
        }
        emit friendListReceived(friends);

    } else if (type == "recv_msg") {
        emit newMessage(json["fromAccount"].toString(),
                        json["nickname"].toString(),
                        json["avatar"].toString(),
                        json["content"].toString(),
                        json["msgType"].toString("text"),
                        json["timestamp"].toString());

    } else if (type == "status_change") {
        emit statusChanged(json["account"].toString(),
                           json["online"].toBool());

    } else if (type == "avatar_sync") {
        emit avatarSynced(json["account"].toString(),
                          json["avatar"].toString());

    } else if (type == "friend_added") {
        emit friendAdded(json["friendInfo"].toObject().toVariantMap());

    } else if (type == "add_friend_resp") {
        QVariantMap friendInfo;
        if (json.contains("friendInfo")) {
            friendInfo = json["friendInfo"].toObject().toVariantMap();
        }
        emit addFriendResult(json["success"].toBool(),
                             json["reason"].toString(),
                             friendInfo);

    } else if (type == "send_ack") {
        emit messageAck(json["toAccount"].toString(),
                        json["status"].toString());
    }
}
