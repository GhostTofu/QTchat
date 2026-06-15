#include "clienthandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
}

ClientHandler::~ClientHandler()
{
    if (m_socket) {
        m_socket->deleteLater();
    }
}

void ClientHandler::sendJson(const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(data);
    m_socket->flush();
}

void ClientHandler::onReadyRead()
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
            qWarning() << "JSON解析失败:" << error.errorString();
            continue;
        }

        QJsonObject json = doc.object();
        QString type = json["type"].toString();

        if (type == "login") {
            emit loginRequested(this, json["account"].toString(),
                                json["password"].toString());
        } else if (type == "register") {
            emit registerRequested(this, json);
        } else if (type == "send_msg") {
            emit sendMessageRequested(m_account,
                                      json["toAccount"].toString(),
                                      json["content"].toString(),
                                      json["msgType"].toString("text"));
        } else if (type == "avatar_update") {
            emit avatarUpdateRequested(m_account, json["avatar"].toString());
        } else if (type == "add_friend") {
            emit addFriendRequested(this,
                                    json["friendAccount"].toString(),
                                    json["groupName"].toString("其他"));
        } else {
            qWarning() << "未知消息类型:" << type;
        }
    }
}

void ClientHandler::onDisconnected()
{
    emit disconnected(this);
}
