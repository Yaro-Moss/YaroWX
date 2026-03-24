#include "VoiceRecordDialog.h"
#include "ConfigManager.h"
#include <QProcess>
#include <QMediaDevices>
#include <QEventLoop>

VoiceRecordDialog::VoiceRecordDialog(QWidget *parent)
    : QDialog(parent)
    , m_recordSeconds(0)
    , m_audioDuration(0)
    , m_isRecording(false)
{
    // 设置音频保存目录
    ConfigManager* configManager = ConfigManager::instance();
    m_audioBaseDir = configManager->dataSavePath() + "/VoiceMessages";
    m_wavDir = m_audioBaseDir + "/wav";
    m_mp3Dir = m_audioBaseDir + "/mp3";

    setupUI();
    setupAudioRecorder();

    // 确保目录存在
    if (!ensureDirectoryExists()) {
        qWarning() << "Failed to create audio directories";
    }
}

VoiceRecordDialog::~VoiceRecordDialog()
{
    if (m_mediaRecorder && m_mediaRecorder->recorderState() == QMediaRecorder::RecordingState) {
        m_mediaRecorder->stop();
    }
}

void VoiceRecordDialog::setupUI()
{
    setWindowTitle("录制语音消息");
    setFixedSize(200, 130);
    setModal(true);

    // 创建UI组件
    m_statusLabel = new QLabel("点击录制语音消息", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: rgb(7, 193, 96);");

    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: rgb(7, 193, 96);");
    m_timerLabel->hide();

    m_cancelButton = new QPushButton("取消", this);
    m_confirmButton = new QPushButton("语音录入", this);

    // 连接信号槽
    connect(m_cancelButton, &QPushButton::clicked, this, &VoiceRecordDialog::cancelRecording);
    connect(m_confirmButton, &QPushButton::clicked, this, &VoiceRecordDialog::startRecording);

    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_timerLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_confirmButton);

    mainLayout->addLayout(buttonLayout);

    // 创建录音计时器
    m_recordTimer = new QTimer(this);
    connect(m_recordTimer, &QTimer::timeout, this, &VoiceRecordDialog::updateTimer);
}

void VoiceRecordDialog::setupAudioRecorder()
{
    // 检查可用音频设备
    auto inputs = QMediaDevices::audioInputs();
    if (inputs.isEmpty()) {
        qWarning() << "没有可用的音频输入设备!";
        return;
    }

    // 创建媒体播放器（用于计算音频时长）
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // 创建捕获会话
    m_captureSession = new QMediaCaptureSession(this);

    // 设置音频输入设备
    m_audioInput = new QAudioInput(this);
    m_audioInput->setDevice(inputs.first());
    m_captureSession->setAudioInput(m_audioInput);

    // 创建录音器
    m_mediaRecorder = new QMediaRecorder(this);
    m_captureSession->setRecorder(m_mediaRecorder);

    // 配置媒体格式 - 使用WAV格式
    QMediaFormat format;
    format.setFileFormat(QMediaFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    m_mediaRecorder->setMediaFormat(format);

    // 设置音频参数
    m_mediaRecorder->setAudioSampleRate(16000);
    m_mediaRecorder->setAudioChannelCount(1);
    m_mediaRecorder->setQuality(QMediaRecorder::NormalQuality);
    m_mediaRecorder->setEncodingMode(QMediaRecorder::ConstantQualityEncoding);

    // 连接信号槽
    connect(m_mediaRecorder, &QMediaRecorder::recorderStateChanged,
            this, &VoiceRecordDialog::onRecordingStateChanged);
    connect(m_mediaRecorder, &QMediaRecorder::errorOccurred,
            this, &VoiceRecordDialog::onRecordingError);
}

bool VoiceRecordDialog::ensureDirectoryExists()
{
    QDir baseDir(m_audioBaseDir);
    QDir wavDir(m_wavDir);
    QDir mp3Dir(m_mp3Dir);

    return baseDir.mkpath(".") && wavDir.mkpath(".") && mp3Dir.mkpath(".");
}

QString VoiceRecordDialog::generateFileName(const QString &extension)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");

    if (extension == "wav") {
        return m_wavDir + "/voice_" + timestamp + ".wav";
    } else {
        return m_mp3Dir + "/voice_" + timestamp + ".mp3";
    }
}

void VoiceRecordDialog::startRecording()
{
    if (m_isRecording) {
        stopRecording();
    } else {
        // 重新检查音频设备
        auto inputs = QMediaDevices::audioInputs();
        if (inputs.isEmpty()) {
            m_statusLabel->setText("未找到麦克风设备");
            m_statusLabel->setStyleSheet("color: #ff4757;");
            return;
        }

        // 确保音频输入设备设置正确
        if (m_audioInput) {
            m_audioInput->setDevice(inputs.first());
        }

        m_recordedWavPath = generateFileName("wav");
        m_mediaRecorder->setOutputLocation(QUrl::fromLocalFile(m_recordedWavPath));

        // 开始录音
        m_mediaRecorder->record();

        // 设置状态标志
        m_isRecording = true;

        // 更新UI
        updateUIForRecording();

        // 添加延迟检查，确保状态同步
        QTimer::singleShot(500, this, [this]() {
            // 如果状态仍然是 StoppedState，说明录音启动失败
            if (m_mediaRecorder->recorderState() == QMediaRecorder::StoppedState) {
                m_statusLabel->setText("录音启动失败，请检查麦克风权限");
                m_statusLabel->setStyleSheet("color: #ff4757;");
                m_isRecording = false;
                updateUIForReady();
            }
        });
    }
}

void VoiceRecordDialog::stopRecording()
{
    if (m_mediaRecorder->recorderState() == QMediaRecorder::RecordingState || m_isRecording) {
        m_mediaRecorder->stop();
        m_recordTimer->stop();

        // 重置状态标志
        m_isRecording = false;

        // 等待一小段时间让文件写入完成
        QTimer::singleShot(200, this, [this]() {
            // 检查WAV文件是否成功创建
            QFile wavFile(m_recordedWavPath);
            bool fileExists = wavFile.exists();
            qint64 fileSize = fileExists ? wavFile.size() : 0;

            if (fileExists && fileSize > 0) {
                if (calculateAudioDuration()) {
                    accept();
                } else {
                    m_statusLabel->setText("无法获取音频时长");
                    m_statusLabel->setStyleSheet("color: #ff4757;");
                    updateUIForReady();
                }
            } else {
                m_statusLabel->setText("录制失败，请重试");
                m_statusLabel->setStyleSheet("color: #ff4757;");
                m_recordedWavPath.clear();
                updateUIForReady();
            }
        });
    } else {
        m_isRecording = false;
        updateUIForReady();
    }
}

void VoiceRecordDialog::updateUIForRecording()
{
    m_confirmButton->setText("确认发送");
    m_confirmButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: rgb(7, 193, 96);"
        "    border: 1px solid rgb(180, 180, 180);"
        "    border-radius: 5px;"
        "    padding: 2px;"
        "    font-size: 10px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgb(5, 150, 75);"
        "    border: 1px solid rgb(150, 150, 150);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgb(3, 120, 60);"
        "    border: 1px solid rgb(120, 120, 120);"
        "}"
        );

    m_statusLabel->setText("正在录制...");
    m_statusLabel->setStyleSheet("color: rgb(7, 193, 96); font-weight: bold;");

    // 显示并启动计时器
    m_timerLabel->show();
    m_recordSeconds = 0;
    updateTimer();
    m_recordTimer->start(1000);
}

void VoiceRecordDialog::updateUIForReady()
{
    m_confirmButton->setText("语音录入");
    m_confirmButton->setStyleSheet(""); // 恢复默认样式

    m_statusLabel->setText("点击录制语音消息");
    m_statusLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: rgb(7, 193, 96);");

    m_timerLabel->hide();
    m_recordTimer->stop();
}

bool VoiceRecordDialog::calculateAudioDuration()
{
    m_mediaPlayer->setSource(QUrl::fromLocalFile(m_recordedWavPath));

    // 等待媒体加载完成
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    auto timerConn = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    auto mediaConn = connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
                             [&loop](QMediaPlayer::MediaStatus status) {
                                 if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
                                     loop.quit();
                                 }
                             });

    timer.start(5000); // 5秒超时
    loop.exec();

    disconnect(mediaConn);
    disconnect(timerConn);

    qint64 duration = m_mediaPlayer->duration();

    if (duration > 0) {
        m_audioDuration = duration / 1000; // 转换为秒
        return true;
    } else {
        // 如果无法通过QMediaPlayer获取时长，使用录制时间作为近似值
        m_audioDuration = m_recordSeconds;
        return true;
    }
}

void VoiceRecordDialog::cancelRecording()
{
    if (m_mediaRecorder->recorderState() == QMediaRecorder::RecordingState) {
        m_mediaRecorder->stop();
        m_recordTimer->stop();
        m_isRecording = false;

        // 删除已录制的文件
        if (!m_recordedWavPath.isEmpty()) {
            QFile::remove(m_recordedWavPath);
            m_recordedWavPath.clear();
        }
    }

    reject();
}

void VoiceRecordDialog::updateTimer()
{
    m_recordSeconds++;
    int minutes = m_recordSeconds / 60;
    int seconds = m_recordSeconds % 60;
    m_timerLabel->setText(QString("%1:%2")
                              .arg(minutes, 2, 10, QLatin1Char('0'))
                              .arg(seconds, 2, 10, QLatin1Char('0')));

    // 限制最长录制时间（300秒=5分钟）
    if (m_recordSeconds >= 300) {
        stopRecording();
    }
}

void VoiceRecordDialog::onRecordingStateChanged(QMediaRecorder::RecorderState state)
{
    switch (state) {
    case QMediaRecorder::RecordingState:
        m_isRecording = true;
        break;
    case QMediaRecorder::StoppedState:
        m_isRecording = false;
        break;
    case QMediaRecorder::PausedState:
        break;
    }
}

void VoiceRecordDialog::onRecordingError(QMediaRecorder::Error error, const QString &errorString)
{
    qWarning() << "录音错误:" << errorString;

    m_statusLabel->setText("录制错误: " + errorString);
    m_statusLabel->setStyleSheet("color: #ff4757;");

    m_recordTimer->stop();
    m_recordedWavPath.clear();
    m_isRecording = false;

    updateUIForReady();
}

// 静态方法：转换WAV到MP3
QString VoiceRecordDialog::convertToMp3(const QString &wavPath, QString *errorMsg)
{
    if (!QFile::exists(wavPath)) {
        if (errorMsg) *errorMsg = "WAV文件不存在: " + wavPath;
        return QString();
    }

    // 生成MP3文件路径
    QFileInfo wavInfo(wavPath);
    QString mp3Path = wavInfo.absolutePath().replace("/wav/", "/mp3/") + "/" + wavInfo.baseName() + ".mp3";

    // 确保MP3目录存在
    QDir mp3Dir(QFileInfo(mp3Path).absolutePath());
    if (!mp3Dir.exists()) {
        mp3Dir.mkpath(".");
    }

    // 检查是否已存在MP3文件
    if (QFile::exists(mp3Path)) {
        return mp3Path;
    }

    // 使用FFmpeg转换WAV到MP3
    QProcess ffmpegProcess;
    QStringList arguments;
    arguments << "-i" << wavPath
              << "-codec:a" << "libmp3lame"
              << "-qscale:a" << "2"
              << "-ac" << "1"
              << "-ar" << "16000"
              << mp3Path;

    ffmpegProcess.start("ffmpeg", arguments);

    if (!ffmpegProcess.waitForStarted(3000)) {
        if (errorMsg) *errorMsg = "无法启动FFmpeg进程";
        return QString();
    }

    if (!ffmpegProcess.waitForFinished(30000)) {
        if (errorMsg) *errorMsg = "FFmpeg转换超时";
        ffmpegProcess.kill();
        return QString();
    }

    if (ffmpegProcess.exitCode() != 0) {
        QString errorOutput = ffmpegProcess.readAllStandardError();
        if (errorMsg) *errorMsg = "FFmpeg转换失败: " + errorOutput;
        return QString();
    }

    // 检查MP3文件是否成功创建
    if (!QFile::exists(mp3Path)) {
        if (errorMsg) *errorMsg = "MP3文件未创建";
        return QString();
    }

    return mp3Path;
}
