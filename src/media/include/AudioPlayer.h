#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFileInfo>
#include <QTimer>
#include <QList>

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    static AudioPlayer* instance();  // 单例访问接口

    // 播放控制，增加 messageId 参数
    void play(const QString &filePath, const qint64 &messageId);
    void pause();
    void resume();
    void stop();
    void setVolume(int volume); // 0-100

    // 状态查询
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;
    QString currentFilePath() const;
    qint64 currentMessageId() const;  // 获取当前播放的消息ID
    int duration() const; // 总时长(毫秒)
    int position() const; // 当前位置(毫秒)

    // 进度控制
    void setPosition(int position);

    // 获取当前波形数据（用于动画）
    QList<int> currentWaveform() const { return m_waveformHeights; }

signals:
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void playbackFinished();
    void positionChanged(int position);
    void durationChanged(int duration);
    void errorOccurred(const QString &errorMessage);
    void waveformUpdated();  // 波形数据更新信号

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onErrorOccurred(QMediaPlayer::Error error, const QString &errorString);
    void updateWaveform();   // 定时更新波形

private:
    explicit AudioPlayer(QObject *parent = nullptr);  // 私有构造
    ~AudioPlayer();

    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QString m_currentFilePath;
    qint64 m_currentMessageId;  // 当前播放的消息ID
    QTimer m_waveformTimer;      // 波形动画定时器
    QList<int> m_waveformHeights; // 当前波形高度（例如4个值）
};

#endif // AUDIOPLAYER_H
