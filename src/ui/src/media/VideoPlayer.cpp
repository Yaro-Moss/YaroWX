#include "VideoPlayer.h"
#include <QStyle>
#include <QTime>
#include <QUrl>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent)
    , m_player(nullptr)
    , m_audioOutput(nullptr)
    , m_videoWidget(nullptr)
    , m_controlPanel(nullptr)
    , m_playPauseBtn(nullptr)
    , m_progressSlider(nullptr)
    , m_speedComboBox(nullptr)
    , m_volumeSlider(nullptr)
    , m_timeLabel(nullptr)
    , m_volumeBtn(nullptr)
    , m_volumePopup(nullptr)
    , m_fullscreenBtn(nullptr)
{
    setupUI();
    m_videoWidget->setVisible(true);

    setupConnections();

    this->setMouseTracking(true);
    m_videoWidget->setMouseTracking(true);
}

VideoPlayer::~VideoPlayer()
{}

void VideoPlayer::setupUI()
{
    setWindowTitle("Qt Video Player");

    m_audioOutput = new QAudioOutput(this);
    m_player = new QMediaPlayer(this);
    m_videoWidget = new QVideoWidget(this);
    m_player->setVideoOutput(m_videoWidget);
    m_player->setAudioOutput(m_audioOutput);

    // 主控制面板
    m_controlPanel = new QWidget(this);
    m_controlPanel->setMaximumHeight(80);
    m_controlPanel->setObjectName("controlPanel");
    m_controlPanel->setStyleSheet(
        "QWidget#controlPanel {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 transparent, stop:0.5 rgba(0, 0, 0, 150), stop:1 rgba(0, 0, 0, 200));"
        "    border-top: 1px solid rgba(255, 255, 255, 80);"
        "    border-radius: 0px;"
        "}");

    // 播放/暂停按钮
    m_playPauseBtn = new QPushButton();
    m_playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playPauseBtn->setFixedSize(40, 40);
    m_playPauseBtn->setObjectName("playPauseBtn");
    m_playPauseBtn->setStyleSheet(
        "QPushButton#playPauseBtn {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius: 0.8, fx:0.5, fy:0.5, stop:0 rgba(255, 255, 255, 200), stop:1 rgba(255, 255, 255, 120));"
        "    border: 2px solid rgba(255, 255, 255, 180);"
        "    border-radius: 20px;"
        "}"
        "QPushButton#playPauseBtn:hover {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius: 0.8, fx:0.5, fy:0.5, stop:0 rgba(255, 255, 255, 230), stop:1 rgba(255, 255, 255, 150));"
        "    border: 2px solid rgba(255, 255, 255, 220);"
        "}");

    // 进度条
    m_progressSlider = new QSlider(Qt::Horizontal);
    m_progressSlider->setObjectName("progressSlider");
    m_progressSlider->setStyleSheet(
        "QSlider#progressSlider::groove:horizontal {"
        "    border: none;"
        "    height: 4px;"
        "    background: rgba(255, 255, 255, 80);"
        "    border-radius: 2px;"
        "    margin: 0px;"
        "}"
        "QSlider#progressSlider::sub-page:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "    stop:0 #2196F3, stop:0.5 #21CBF3, stop:1 #03A9F4);"
        "    border-radius: 2px;"
        "}"
        "QSlider#progressSlider::handle:horizontal {"
        "    background: white;"
        "    border: 2px solid #2196F3;"
        "    width: 16px;"
        "    margin: -6px 0;"
        "    border-radius: 8px;"
        "}"
        "QSlider#progressSlider::handle:horizontal:hover {"
        "    background: #2196F3;"
        "    border: 2px solid white;"
        "    width: 18px;"
        "}");

    // 时间标签
    m_timeLabel = new QLabel("00:00 / 00:00");
    m_timeLabel->setStyleSheet(
        "QLabel {"
        "    color: rgba(255, 255, 255, 200);"
        "    font-size: 12px;"
        "    font-family: Arial;"
        "    background: transparent;"
        "}");

    // 播放速度选择框
    m_speedComboBox = new QComboBox();
    m_speedComboBox->addItems({"0.5x", "1.0x", "1.25x", "1.5x", "2.0x"});
    m_speedComboBox->setCurrentText("1.0x");
    m_speedComboBox->setFixedWidth(70);
    m_speedComboBox->setObjectName("speedComboBox");
    m_speedComboBox->setStyleSheet(
        "QComboBox#speedComboBox {"
        "    background: rgba(255, 255, 255, 150);"
        "    border: 1px solid rgba(255, 255, 255, 100);"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    color: black;"
        "    font-size: 12px;"
        "}"
        "QComboBox#speedComboBox:hover {"
        "    background: rgba(255, 255, 255, 180);"
        "    border: 1px solid rgba(255, 255, 255, 150);"
        "}"
        "QComboBox#speedComboBox::drop-down {"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QComboBox#speedComboBox::down-arrow {"
        "    image: none;"
        "    border-left: 5px solid transparent;"
        "    border-right: 5px solid transparent;"
        "    border-top: 5px solid rgba(0, 0, 0, 150);"
        "    width: 0px;"
        "    height: 0px;"
        "}"
        "QComboBox#speedComboBox QAbstractItemView {"
        "    background: rgba(0, 0, 0, 220);"
        "    border: 1px solid rgba(255, 255, 255, 100);"
        "    border-radius: 4px;"
        "    color: white;"
        "    selection-background-color: #2196F3;"
        "    outline: none;"
        "}");

    // 音量按钮
    m_volumeBtn = new QPushButton();
    updateVolumeIcon(50);
    m_volumeBtn->setFixedSize(32, 32);
    m_volumeBtn->setObjectName("volumeBtn");
    m_volumeBtn->setStyleSheet(
        "QPushButton#volumeBtn {"
        "    background: rgba(255, 255, 255, 120);"
        "    border: 1px solid rgba(255, 255, 255, 80);"
        "    border-radius: 16px;"
        "}"
        "QPushButton#volumeBtn:hover {"
        "    background: rgba(255, 255, 255, 180);"
        "    border: 1px solid rgba(255, 255, 255, 120);"
        "}");

    // 音量滑块
    m_volumeSlider = new QSlider(Qt::Vertical);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    m_volumeSlider->setFixedSize(30, 100);
    m_volumeSlider->setObjectName("volumeSlider");
    m_volumeSlider->setStyleSheet(
        "QSlider#volumeSlider::groove:vertical {"
        "    border: none;"
        "    width: 4px;"
        "    background: rgba(255, 255, 255, 80);"
        "    border-radius: 2px;"
        "    margin: 0px;"
        "}"
        "QSlider#volumeSlider::add-page:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "    stop:0 #2196F3, stop:0.5 #21CBF3, stop:1 #03A9F4);"
        "    border-radius: 2px;"
        "}"
        "QSlider#volumeSlider::handle:vertical {"
        "    background: white;"
        "    border: 2px solid #2196F3;"
        "    height: 12px;"
        "    margin: 0 -4px;"
        "    border-radius: 6px;"
        "}"
        "QSlider#volumeSlider::handle:vertical:hover {"
        "    background: #2196F3;"
        "    border: 2px solid white;"
        "}");

    // 音量滑块容器（用于悬浮显示）
    m_volumePopup = new QWidget(this);
    m_volumePopup->setObjectName("volumePopup");
    m_volumePopup->setFixedSize(40, 120);
    m_volumePopup->setStyleSheet(
        "QWidget#volumePopup {"
        "    background: rgba(0, 0, 0, 200);"
        "    border: 1px solid rgba(255, 255, 255, 100);"
        "    border-radius: 8px;"
        "}");
    m_volumePopup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_volumePopup->setAttribute(Qt::WA_TranslucentBackground);
    m_volumePopup->hide();

    QVBoxLayout *volumePopupLayout = new QVBoxLayout(m_volumePopup);
    volumePopupLayout->addWidget(m_volumeSlider, 0, Qt::AlignHCenter);
    volumePopupLayout->setContentsMargins(5, 10, 5, 10);

    // 全屏按钮
    m_fullscreenBtn = new QPushButton();
    m_fullscreenBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    m_fullscreenBtn->setFixedSize(32, 32);
    m_fullscreenBtn->setObjectName("fullscreenBtn");
    m_fullscreenBtn->setStyleSheet(
        "QPushButton#fullscreenBtn {"
        "    background: rgba(255, 255, 255, 120);"
        "    border: 1px solid rgba(255, 255, 255, 80);"
        "    border-radius: 16px;"
        "}"
        "QPushButton#fullscreenBtn:hover {"
        "    background: rgba(255, 255, 255, 180);"
        "    border: 1px solid rgba(255, 255, 255, 120);"
        "}");

    // 控制面板布局
    QHBoxLayout *topControlLayout = new QHBoxLayout();
    topControlLayout->addWidget(m_progressSlider, 1);
    topControlLayout->addWidget(m_timeLabel);
    topControlLayout->setSpacing(10);

    QHBoxLayout *bottomControlLayout = new QHBoxLayout();
    bottomControlLayout->addWidget(m_playPauseBtn);
    bottomControlLayout->addStretch(1);
    bottomControlLayout->addWidget(m_speedComboBox);
    bottomControlLayout->addWidget(m_volumeBtn);
    bottomControlLayout->addWidget(m_fullscreenBtn);
    bottomControlLayout->setSpacing(8);

    QVBoxLayout *controlLayout = new QVBoxLayout(m_controlPanel);
    controlLayout->addLayout(topControlLayout);
    controlLayout->addLayout(bottomControlLayout);
    controlLayout->setSpacing(8);
    controlLayout->setContentsMargins(20, 10, 20, 15);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_videoWidget, 1);
    mainLayout->addWidget(m_controlPanel);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
}

void VideoPlayer::setupConnections()
{
    connect(m_playPauseBtn, &QPushButton::clicked, this, &VideoPlayer::togglePlayPause);
    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoPlayer::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &VideoPlayer::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &VideoPlayer::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error, const QString&){});
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [](QMediaPlayer::MediaStatus){});

    connect(m_progressSlider, &QSlider::sliderMoved, this, &VideoPlayer::seekVideo);
    connect(m_speedComboBox, &QComboBox::currentTextChanged, this, &VideoPlayer::changePlaybackRate);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &VideoPlayer::changeVolume);
    connect(m_volumeBtn, &QPushButton::clicked, this, &VideoPlayer::toggleMute);
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &VideoPlayer::toggleFullscreen);

    // 音量按钮悬浮事件
    m_volumeBtn->installEventFilter(this);
    m_volumeSlider->installEventFilter(this);
    m_volumePopup->installEventFilter(this);
    m_videoWidget->installEventFilter(this);
}

void VideoPlayer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void VideoPlayer::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

bool VideoPlayer::eventFilter(QObject *obj, QEvent *event)
{
    // 视频widget点击播放/暂停
    if (obj == m_videoWidget && event->type() == QEvent::MouseButtonPress) {
        togglePlayPause();
        return true;
    }

    // 音量按钮悬浮显示音量滑块
    if (obj == m_volumeBtn) {
        if (event->type() == QEvent::Enter) {
            showVolumePopup();
        } else if (event->type() == QEvent::Leave) {
            // 延迟隐藏，让用户有机会移动到音量滑块上
            QTimer::singleShot(300, this, [this]() {
                if (!m_volumePopup->underMouse() && !m_volumeSlider->underMouse()) {
                    hideVolumePopup();
                }
            });
        }
    }

    // 音量滑块悬浮处理
    if (obj == m_volumeSlider || obj == m_volumePopup) {
        if (event->type() == QEvent::Leave) {
            QTimer::singleShot(300, this, [this]() {
                if (!m_volumeBtn->underMouse()) {
                    hideVolumePopup();
                }
            });
        }
    }

    return QWidget::eventFilter(obj, event);
}

void VideoPlayer::togglePlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    } else {
        m_player->play();
        m_playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
}

void VideoPlayer::onPositionChanged(qint64 position)
{
    if (!m_progressSlider->isSliderDown()) {
        m_progressSlider->setValue(position);
    }

    // 更新时间标签
    QTime currentTime(0, 0, 0, 0);
    currentTime = currentTime.addMSecs(position);
    QTime totalTime(0, 0, 0, 0);
    totalTime = totalTime.addMSecs(m_player->duration());

    QString format = "mm:ss";
    if (m_player->duration() > 3600000) { // 超过1小时
        format = "hh:mm:ss";
    }

    m_timeLabel->setText(currentTime.toString(format) + " / " + totalTime.toString(format));
}

void VideoPlayer::onDurationChanged(qint64 duration)
{
    m_progressSlider->setRange(0, duration);
}

void VideoPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        m_playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        m_playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void VideoPlayer::seekVideo(int position)
{
    m_player->setPosition(position);
}

void VideoPlayer::changePlaybackRate(const QString &rate)
{
    m_player->setPlaybackRate(QStringView(rate).left(rate.length() - 1).toDouble());
}

void VideoPlayer::changeVolume(int volume)
{
    m_audioOutput->setVolume(volume / 100.0);
    updateVolumeIcon(volume);

    // 如果之前是静音状态，取消静音
    if (m_audioOutput->isMuted() && volume > 0) {
        m_audioOutput->setMuted(false);
    }
}

void VideoPlayer::toggleMute()
{
    bool muted = !m_audioOutput->isMuted();
    m_audioOutput->setMuted(muted);

    if (muted) {
        m_volumeBtn->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    } else {
        updateVolumeIcon(m_volumeSlider->value());
    }
}

void VideoPlayer::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
        m_fullscreenBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    } else {
        showFullScreen();
        m_fullscreenBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
    }
}

void VideoPlayer::updateVolumeIcon(int volume)
{
    if (volume == 0) {
        m_volumeBtn->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    } else if (volume < 33) {
        m_volumeBtn->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    } else if (volume < 66) {
        m_volumeBtn->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    } else {
        m_volumeBtn->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    }
}

void VideoPlayer::showVolumePopup()
{
    QPoint globalPos = m_volumeBtn->mapToGlobal(QPoint(0, 0));
    m_volumePopup->move(globalPos.x() - (m_volumePopup->width() - m_volumeBtn->width()) / 2,
                        globalPos.y() - m_volumePopup->height() - 5);
    m_volumePopup->show();
}

void VideoPlayer::hideVolumePopup()
{
    m_volumePopup->hide();
}


void VideoPlayer::loadVideo(const QString &path)
{
    videoUrl = path;

    // 判断是本地文件还是网络URL
    if (path.startsWith("http://") || path.startsWith("https://")) {
        m_player->setSource(QUrl(path));
    } else {
        m_player->setSource(QUrl::fromLocalFile(videoUrl));
        qDebug()<<"播放地址"<<videoUrl;
    }
    if (m_videoWidget) {
        m_videoWidget->update();
        m_videoWidget->repaint();
    }
}


void VideoPlayer::stop()
{
    if (m_player) {
        m_player->stop();
    }
}
