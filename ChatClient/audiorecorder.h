#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QAudioInput>
#include <QFile>

class AudioRecorder : public QObject
{
    Q_OBJECT
public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder();

    // 开始录音，16kHz 单声道 PCM
    bool startRecord();

    // 停止录音，返回保存的文件路径
    QString stopRecord();

    // 是否正在录音
    bool isRecording() const;

    // 录音文件路径
    QString filePath() const { return m_filePath; }

signals:
    void recordingStarted();
    void recordingStopped(const QString &filePath);
    void errorOccurred(const QString &error);

private:
    QAudioInput *m_audioInput;
    QFile m_file;
    QString m_filePath;
    bool m_recording;
};

#endif // AUDIORECORDER_H
