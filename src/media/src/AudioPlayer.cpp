#include "AudioPlayer.h"
#include <QDebug>
#include <QRandomGenerator>

// 静态实例指针
static AudioPlayer* s_instance = nullptr;

AudioPlayer* AudioPlayer::instance()
{
    if (!s_instance) {
        s_instance = new AudioPlayer();
    }
    return s_instance;
}

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
    , m_currentFilePath("")
    , m_currentMessageId(-1)
{
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(1);

    // 初始化波形数据（默认静态值）
    m_waveformHeights = {8, 12, 16, 12};

    // 设置波形更新定时器（50ms间隔）
    m_waveformTimer.setInterval(50);
    connect(&m_waveformTimer, &QTimer::timeout, this, &AudioPlayer::updateWaveform);

    // 连接信号
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &AudioPlayer::onMediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &AudioPlayer::onPlaybackStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged,
            this, &AudioPlayer::onPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
            this, &AudioPlayer::onDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred,
            this, &AudioPlayer::onErrorOccurred);
}

AudioPlayer::~AudioPlayer()
{
    stop();
}

void AudioPlayer::play(const QString &filePath, const qint64 &messageId)
{
    if (filePath.isEmpty()) {
        emit errorOccurred("文件路径为空");
        return;
    }

    if (!QFile::exists(filePath)) {
        emit errorOccurred("文件不存在: " + filePath);
        return;
    }

    // 如果点击的是同一条消息，则停止播放（或者暂停，根据需要选择）
    if (m_currentMessageId == messageId && isPlaying()) {
        pause();   // 或 stop()，这里我们选择暂停
        return;
    }

    // 如果正在播放其他消息，先停止
    if (m_mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        stop();
    }

    // 设置新的媒体源
    if (m_currentFilePath != filePath) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        m_currentFilePath = filePath;
    }
    m_currentMessageId = messageId;

    // 开始播放
    m_mediaPlayer->play();
    m_waveformTimer.start();  // 启动波形动画
}

void AudioPlayer::pause()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
        m_waveformTimer.stop();
    }
}

void AudioPlayer::resume()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        m_mediaPlayer->play();
        m_waveformTimer.start();
    }
}

void AudioPlayer::stop()
{
    m_mediaPlayer->stop();
    m_waveformTimer.stop();
    m_currentFilePath.clear();
    m_currentMessageId = -1;
    // 恢复默认静态波形
    m_waveformHeights = {8, 12, 16, 12};
}

void AudioPlayer::setVolume(int volume)
{
    // 将0-100的整数音量转换为0.0-1.0的浮点数
    float normalizedVolume = qBound(0, volume, 100) / 100.0f;
    m_audioOutput->setVolume(normalizedVolume);
}

bool AudioPlayer::isPlaying() const
{
    return m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState;
}

bool AudioPlayer::isPaused() const
{
    return m_mediaPlayer->playbackState() == QMediaPlayer::PausedState;
}

bool AudioPlayer::isStopped() const
{
    return m_mediaPlayer->playbackState() == QMediaPlayer::StoppedState;
}

QString AudioPlayer::currentFilePath() const
{
    return m_currentFilePath;
}

int AudioPlayer::duration() const
{
    return m_mediaPlayer->duration();
}

int AudioPlayer::position() const
{
    return m_mediaPlayer->position();
}

void AudioPlayer::setPosition(int position)
{
    m_mediaPlayer->setPosition(position);
}

qint64 AudioPlayer::currentMessageId() const
{
    return m_currentMessageId;
}

void AudioPlayer::updateWaveform()
{
    // 模拟波形变化：随机生成4个高度值（范围6~20）
    m_waveformHeights.clear();
    for (int i = 0; i < 4; ++i) {
        m_waveformHeights.append(6 + QRandomGenerator::global()->bounded(15));
    }
    emit waveformUpdated();
}

void AudioPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadedMedia:
        qDebug() << "媒体加载完成:" << m_currentFilePath;
        break;
    case QMediaPlayer::EndOfMedia:
        emit playbackFinished();
        qDebug() << "播放完成:" << m_currentFilePath;
        stop();  // 播放结束自动停止（清除状态）
        break;
    case QMediaPlayer::InvalidMedia:
        emit errorOccurred("无效的媒体文件: " + m_currentFilePath);
        break;
    default:
        break;
    }
}

void AudioPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        emit playbackStarted();
        break;
    case QMediaPlayer::PausedState:
        emit playbackPaused();
        break;
    case QMediaPlayer::StoppedState:
        emit playbackStopped();
        break;
    }
}

void AudioPlayer::onPositionChanged(qint64 position)
{
    emit positionChanged(position);
}

void AudioPlayer::onDurationChanged(qint64 duration)
{
    emit durationChanged(duration);
}

void AudioPlayer::onErrorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    QString errorMsg;
    switch (error) {
    case QMediaPlayer::NoError:
        return;
    case QMediaPlayer::ResourceError:
        errorMsg = "资源错误: " + errorString;
        break;
    case QMediaPlayer::FormatError:
        errorMsg = "格式错误: " + errorString;
        break;
    case QMediaPlayer::NetworkError:
        errorMsg = "网络错误: " + errorString;
        break;
    case QMediaPlayer::AccessDeniedError:
        errorMsg = "访问被拒绝: " + errorString;
        break;
    default:
        errorMsg = "未知错误: " + errorString;
        break;
    }

    emit errorOccurred(errorMsg);
    qWarning() << "音频播放错误:" << errorMsg;
}
