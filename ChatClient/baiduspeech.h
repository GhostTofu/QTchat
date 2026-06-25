#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#ifndef BAIDUSPEECH_H
#define BAIDUSPEECH_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class BaiduSpeech : public QObject
{
    Q_OBJECT
public:
    explicit BaiduSpeech(QObject *parent = nullptr);

    // 从 key 文件读取 AK/SK，获取 access_token
    // keyFile 格式: 第一行 APIkey:xxx，第二行 Secret Key:xxx
    void requestToken(const QString &keyFilePath);

    // 语音识别：上传 wav 文件，返回识别文本
    // 文件需为 16kHz 单声道 PCM 格式
    void recognize(const QString &audioFilePath);

    // 是否已获取 token
    bool hasToken() const { return !m_accessToken.isEmpty(); }

signals:
    // token 获取完成
    void tokenReady(const QString &accessToken);
    void tokenError(const QString &error);

    // 识别结果
    void recognitionResult(const QString &text);
    void recognitionError(const QString &error);

private slots:
    void onTokenReply(QNetworkReply *reply);
    void onRecognizeReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_network;
    QString m_accessToken;
    QString m_apiKey;
    QString m_secretKey;
};

#endif // BAIDUSPEECH_H
