#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "chatserver.h"
#include "clienthandler.h"
#include "databasemanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

ChatServer::ChatServer(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &ChatServer::onNewConnection);
}

ChatServer::~ChatServer()
{
    stopServer();
}

bool ChatServer::isRunning() const
{
    return m_server && m_server->isListening();
}

int ChatServer::clientCount() const
{
    return m_clients.size();
}

bool ChatServer::startServer(int port)
{
    if (!DatabaseManager::instance()->initDatabase()) {
        emit errorOccurred("数据库初始化失败!");
        return false;
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit errorOccurred("服务器启动失败: " + m_server->errorString());
        return false;
    }

    emit logMessage("===== 聊天服务器已启动 =====");
    emit logMessage("监听端口: " + QString::number(port));
    emit logMessage("等待客户端连接...");
    emit serverStarted(port);
    return true;
}

void ChatServer::stopServer()
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        DatabaseManager::instance()->setUserOnline(it.key(), false);
    }
    m_server->close();
    qDeleteAll(m_handlers);
    m_handlers.clear();
    m_clients.clear();
    emit logMessage("服务器已停止");
    emit serverStopped();
}

void ChatServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        ClientHandler *handler = new ClientHandler(socket, this);

        // 连接所有信号
        connect(handler, &ClientHandler::disconnected,
                this, &ChatServer::onClientDisconnected);
        connect(handler, &ClientHandler::loginRequested,
                this, &ChatServer::onLoginRequested);
        connect(handler, &ClientHandler::registerRequested,
                this, &ChatServer::onRegisterRequested);
        connect(handler, &ClientHandler::sendMessageRequested,
                this, &ChatServer::onSendMessage);
        connect(handler, &ClientHandler::avatarUpdateRequested,
                this, &ChatServer::onAvatarUpdate);
        connect(handler, &ClientHandler::addFriendRequested,
                this, &ChatServer::onAddFriend);

        m_handlers.append(handler);
        QString addr = socket->peerAddress().toString();
        int port = socket->peerPort();
        emit logMessage("新连接: " + addr + ":" + QString::number(port));
        emit clientConnected(addr, port);
    }
}

void ChatServer::onClientDisconnected(ClientHandler *handler)
{
    QString account = handler->getAccount();
    if (!account.isEmpty()) {
        m_clients.remove(account);
        DatabaseManager::instance()->setUserOnline(account, false);
        broadcastStatusToFriends(account, false);
        emit logMessage("用户离线: " + account);
        emit clientDisconnected(account);
    }
    m_handlers.removeOne(handler);
    handler->deleteLater();
}

// ==================== 业务处理 ====================

void ChatServer::onLoginRequested(ClientHandler *handler, const QString &account,
                                   const QString &password)
{
    QJsonObject resp;
    resp["type"] = "login_resp";

    QVariantMap userInfo = DatabaseManager::instance()->loginUser(account, password);

    if (userInfo["success"].toBool()) {
        resp["success"] = true;
        resp["account"] = userInfo["account"].toString();
        resp["nickname"] = userInfo["nickname"].toString();
        resp["gender"] = userInfo["gender"].toString();
        resp["avatar"] = userInfo["avatar"].toString();

        // 绑定账号
        handler->m_account = account;
        m_clients[account] = handler;
        DatabaseManager::instance()->setUserOnline(account, true);

        emit logMessage("用户登录: " + account + " " + userInfo["nickname"].toString());
        emit clientLoggedIn(account, userInfo["nickname"].toString());
    } else {
        resp["success"] = false;
        if (DatabaseManager::instance()->userExists(account)) {
            resp["reason"] = "密码错误";
        } else {
            resp["reason"] = "账号不存在";
        }
        emit logMessage("登录失败: " + account + " - " + resp["reason"].toString());
    }

    handler->sendJson(resp);

    // 登录成功后发送好友列表 + 广播上线
    if (resp["success"].toBool()) {
        sendFriendList(handler, account);
        broadcastStatusToFriends(account, true);
    }
}

void ChatServer::onRegisterRequested(ClientHandler *handler, const QJsonObject &data)
{
    QString account = data["account"].toString();
    QString nickname = data["nickname"].toString();
    QString password = data["password"].toString();
    QString gender = data["gender"].toString();
    QString avatar = data["avatar"].toString();

    QJsonObject resp;
    resp["type"] = "register_resp";

    if (DatabaseManager::instance()->userExists(account)) {
        resp["success"] = false;
        resp["reason"] = "账号已存在";
    } else {
        bool ok = DatabaseManager::instance()->registerUser(account, nickname,
                                                              password, gender, avatar);
        resp["success"] = ok;
        if (!ok) {
            resp["reason"] = "数据库写入失败";
        }
    }

    handler->sendJson(resp);
    emit logMessage("注册请求: " + account + (resp["success"].toBool() ? " 成功" : " 失败"));
}

void ChatServer::onSendMessage(const QString &fromAccount, const QString &toAccount,
                                 const QString &content, const QString &msgType)
{
    // 保存到数据库
    DatabaseManager::instance()->saveMessage(fromAccount, toAccount, content, msgType);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 转发给目标
    ClientHandler *target = m_clients.value(toAccount, nullptr);
    if (target) {
        QJsonObject fwd;
        fwd["type"] = "recv_msg";
        fwd["fromAccount"] = fromAccount;
        QVariantMap senderInfo = DatabaseManager::instance()->getUserInfo(fromAccount);
        fwd["nickname"] = senderInfo["nickname"].toString();
        fwd["avatar"] = senderInfo["avatar"].toString();
        fwd["content"] = content;
        fwd["msgType"] = msgType;
        fwd["timestamp"] = timestamp;
        target->sendJson(fwd);
    }

    // 回执给发送者
    ClientHandler *sender = m_clients.value(fromAccount, nullptr);
    if (sender) {
        QJsonObject ack;
        ack["type"] = "send_ack";
        ack["toAccount"] = toAccount;
        ack["status"] = (target != nullptr) ? "delivered" : "offline";
        sender->sendJson(ack);
    }

    emit logMessage("消息: " + fromAccount + " -> " + toAccount + " [" + msgType + "]");
    emit messageRelayed(fromAccount, toAccount, content, msgType);
}

void ChatServer::onAvatarUpdate(const QString &account, const QString &avatar)
{
    DatabaseManager::instance()->updateUserAvatar(account, avatar);

    // 通知所有在线好友
    QVariantList friends = DatabaseManager::instance()->getFriendList(account);
    for (const QVariant &f : friends) {
        QString friendAccount = f.toMap()["account"].toString();
        ClientHandler *target = m_clients.value(friendAccount, nullptr);
        if (target) {
            QJsonObject sync;
            sync["type"] = "avatar_sync";
            sync["account"] = account;
            sync["avatar"] = avatar;
            target->sendJson(sync);
        }
    }
    emit logMessage("头像更新: " + account);
}

void ChatServer::onAddFriend(ClientHandler *handler, const QString &friendAccount,
                               const QString &groupName)
{
    QString myAccount = handler->getAccount();
    QJsonObject resp;
    resp["type"] = "add_friend_resp";

    if (!DatabaseManager::instance()->userExists(friendAccount)) {
        resp["success"] = false;
        resp["reason"] = "该账号不存在";
    } else {
        DatabaseManager::instance()->addFriend(myAccount, friendAccount, groupName);
        DatabaseManager::instance()->addFriend(friendAccount, myAccount, groupName);
        resp["success"] = true;

        QVariantMap info = DatabaseManager::instance()->getUserInfo(friendAccount);
        info["groupName"] = groupName;
        resp["friendInfo"] = QJsonObject::fromVariantMap(info);

        // 通知对方有新好友
        ClientHandler *target = m_clients.value(friendAccount, nullptr);
        if (target) {
            QJsonObject notify;
            notify["type"] = "friend_added";
            QVariantMap myInfo = DatabaseManager::instance()->getUserInfo(myAccount);
            myInfo["groupName"] = groupName;
            notify["friendInfo"] = QJsonObject::fromVariantMap(myInfo);
            target->sendJson(notify);

            // 也刷新对方的好友列表
            sendFriendList(target, friendAccount);
        }

        // 刷新自己的好友列表
        sendFriendList(handler, myAccount);
    }

    handler->sendJson(resp);
    emit logMessage("添加好友: " + myAccount + " <-> " + friendAccount
                    + (resp["success"].toBool() ? " 成功" : " 失败"));
}

// ==================== 辅助方法 ====================

ClientHandler* ChatServer::findHandler(const QString &account)
{
    return m_clients.value(account, nullptr);
}

void ChatServer::broadcastStatusToFriends(const QString &account, bool online)
{
    QVariantList friends = DatabaseManager::instance()->getFriendList(account);
    for (const QVariant &f : friends) {
        QString friendAccount = f.toMap()["account"].toString();
        ClientHandler *target = m_clients.value(friendAccount, nullptr);
        if (target) {
            QJsonObject sc;
            sc["type"] = "status_change";
            sc["account"] = account;
            sc["online"] = online;
            target->sendJson(sc);
        }
    }
}

void ChatServer::sendFriendList(ClientHandler *handler, const QString &account)
{
    QVariantList friends = DatabaseManager::instance()->getFriendList(account);
    QJsonObject flist;
    flist["type"] = "friend_list";
    QJsonArray arr;
    for (const QVariant &f : friends) {
        arr.append(QJsonObject::fromVariantMap(f.toMap()));
    }
    flist["friends"] = arr;
    handler->sendJson(flist);
}
