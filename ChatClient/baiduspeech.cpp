#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "baiduspeech.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QSslError>
#include <QDebug>
#include <QFileInfo>

BaiduSpeech::BaiduSpeech(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
    // MinGW 缺少 OpenSSL 时，忽略 SSL 错误以支持 HTTPS 请求
    connect(m_network, &QNetworkAccessManager::sslErrors,
            this, [](QNetworkReply *reply, const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        reply->ignoreSslErrors();
    });
}

void BaiduSpeech::requestToken(const QString &keyFilePath)
{
    // 读取 key 文件
    QFile file(keyFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit tokenError(QString("无法打开 key 文件: %1").arg(keyFilePath));
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QStringList lines = content.split('\n');
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        // 支持英文冒号 : 和中文冒号 \xef\xbc\x9a
        int colonPos = qMax(trimmed.indexOf(':'), trimmed.indexOf(QChar(0xFF1A)));
        if (colonPos < 0) continue;

        QString keyPart = trimmed.left(colonPos).trimmed();
        QString valuePart = trimmed.mid(colonPos + 1).trimmed();

        if (keyPart == "APIkey") {
            m_apiKey = valuePart;
        } else if (keyPart == "Secret Key") {
            m_secretKey = valuePart;
        }
    }

    if (m_apiKey.isEmpty() || m_secretKey.isEmpty()) {
        emit tokenError("key 文件格式错误，需要 APIkey 和 Secret Key");
        return;
    }

    // 构造 OAuth2 请求
    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", m_apiKey);
    query.addQueryItem("client_secret", m_secretKey);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_network->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onTokenReply(reply);
    });

    qDebug() << "[BaiduSpeech] 正在获取 access_token...";
}

void BaiduSpeech::onTokenReply(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit tokenError("网络错误: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (obj.contains("access_token")) {
        m_accessToken = obj["access_token"].toString();
        int expiresIn = obj["expires_in"].toInt();
        qDebug() << "[BaiduSpeech] Token 获取成功, 有效期" << expiresIn << "秒";
        emit tokenReady(m_accessToken);
    } else {
        QString err = obj["error_description"].toString();
        if (err.isEmpty()) err = obj["error"].toString();
        emit tokenError("Token 获取失败: " + err);
    }
}

void BaiduSpeech::recognize(const QString &audioFilePath)
{
    if (m_accessToken.isEmpty()) {
        emit recognitionError("尚未获取 access_token");
        return;
    }

    QFile file(audioFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit recognitionError("无法打开音频文件: " + audioFilePath);
        return;
    }

    QByteArray audioData = file.readAll();
    file.close();

    if (audioData.isEmpty()) {
        emit recognitionError("音频文件为空");
        return;
    }

    // 构造识别请求
    QString urlStr = QString(
        "http://vop.baidu.com/server_api?dev_pid=1537&cuid=QTchat&token=%1")
        .arg(m_accessToken);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", "audio/pcm;rate=16000");

    QNetworkReply *reply = m_network->post(request, audioData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onRecognizeReply(reply);
    });

    qDebug() << "[BaiduSpeech] 正在识别语音, 文件大小:" << audioData.size() << "bytes";
}

void BaiduSpeech::onRecognizeReply(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit recognitionError("网络错误: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "[BaiduSpeech] 识别返回:" << data;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    // 检查错误
    if (obj.contains("err_no") && obj["err_no"].toInt() != 0) {
        emit recognitionError("识别失败: " + obj["err_msg"].toString());
        return;
    }

    // 提取识别文本
    QJsonArray resultArr = obj["result"].toArray();
    if (resultArr.isEmpty()) {
        emit recognitionResult("");
    } else {
        QString text = resultArr.at(0).toString();
        emit recognitionResult(text);
    }
}
