#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "audiorecorder.h"
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent), m_audioInput(nullptr), m_recording(false)
{
}

AudioRecorder::~AudioRecorder()
{
    stopRecord();
}

bool AudioRecorder::startRecord()
{
    if (m_recording) return true;

    // 设置录音格式：16kHz 单声道 16bit PCM
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // 检查设备是否支持此格式
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format)) {
        format = info.nearestFormat(format);
        qDebug() << "[AudioRecorder] 使用最接近的格式:"
                 << format.sampleRate() << "Hz,"
                 << format.channelCount() << "ch";
    }

    // 创建录音设备
    m_audioInput = new QAudioInput(format, this);

    // 准备文件
    m_filePath = QDir::temp().absoluteFilePath(
        QString("voice_%1.pcm").arg(QDateTime::currentMSecsSinceEpoch()));
    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred("无法创建录音文件: " + m_filePath);
        return false;
    }

    m_recording = true;
    m_audioInput->start(&m_file);
    emit recordingStarted();
    qDebug() << "[AudioRecorder] 开始录音 →" << m_filePath;
    return true;
}

QString AudioRecorder::stopRecord()
{
    if (!m_recording) return QString();

    m_audioInput->stop();
    m_file.close();

    delete m_audioInput;
    m_audioInput = nullptr;
    m_recording = false;

    qDebug() << "[AudioRecorder] 录音完成:" << m_filePath
             << "大小:" << QFileInfo(m_filePath).size() << "bytes";
    emit recordingStopped(m_filePath);
    return m_filePath;
}

bool AudioRecorder::isRecording() const
{
    return m_recording;
}
