#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTimer>

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();
    void loadVideo(const QString &path);
    void stop();


private slots:
    void togglePlayPause();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void seekVideo(int position);
    void changePlaybackRate(const QString &rate);
    void changeVolume(int volume);
    void toggleMute();
    void toggleFullscreen();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    void setupConnections();
    void updateVolumeIcon(int volume);
    void showVolumePopup();
    void hideVolumePopup();

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QVideoWidget *m_videoWidget;
    QWidget *m_controlPanel;
    QPushButton *m_playPauseBtn;
    QSlider *m_progressSlider;
    QComboBox *m_speedComboBox;
    QSlider *m_volumeSlider;

    // 新添加的成员变量
    QLabel *m_timeLabel;
    QPushButton *m_volumeBtn;
    QWidget *m_volumePopup;
    QPushButton *m_fullscreenBtn;

    QString videoUrl;
};

#endif // VIDEOPLAYER_H
