#include "VideoProcessor.h"
#include "ConfigManager.h"
#include "VideoProcessingTask.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

VideoProcessor::VideoProcessor(QObject *parent)
    : QObject(parent)
    , m_thumbnailSize(320, 240)  // 默认缩略图大小
{
    ConfigManager* configManager = ConfigManager::instance();
    QDir baseDir(configManager->dataSavePath());

    // 设置原视频和缩略图保存路径
    m_videoSavePath = baseDir.absoluteFilePath("videos");
    m_thumbnailSavePath = baseDir.absoluteFilePath("video_thumbnails");

    // 确保目录存在
    baseDir.mkpath("videos");
    baseDir.mkpath("video_thumbnails");

    // 设置线程池
    int idealThreadCount = QThread::idealThreadCount();
    m_threadPool.setMaxThreadCount(qMax(2, idealThreadCount));
}

VideoProcessor::~VideoProcessor()
{
    m_threadPool.waitForDone(5000); // 最多等待5秒
}

void VideoProcessor::processVideo(const qint64 conversationId, const QString &sourceVideoPath, int priority)
{
    processVideos(conversationId, QStringList() << sourceVideoPath, priority);
}

void VideoProcessor::processVideos(const qint64 conversationId, const QStringList &sourceVideoPaths, int priority)
{
    for (const QString &videoPath : sourceVideoPaths) {
        // 创建视频处理任务
        VideoProcessingTask *task = new VideoProcessingTask(conversationId,
                                                            videoPath,
                                                            m_videoSavePath,
                                                            m_thumbnailSavePath,
                                                            m_thumbnailSize,
                                                            priority);

        // 连接任务完成信号
        connect(task, &VideoProcessingTask::finished,
                this, &VideoProcessor::onTaskFinished);

        // 提交到线程池
        m_threadPool.start(task);
    }
}

void VideoProcessor::onTaskFinished(const qint64 conversationId,
                                    bool success,
                                    const QString &sourcePath,
                                    const QString &originalPath,
                                    const QString &thumbnailPath,
                                    const QString &errorMessage)
{
    if (success) {
        emit videoProcessed(conversationId, originalPath, thumbnailPath, true);
    } else {
        emit videoProcessingFailed(conversationId, sourcePath, errorMessage);
    }
}

void VideoProcessor::setVideoSavePath(const QString &path)
{
    m_videoSavePath = path;
    QDir dir;
    dir.mkpath(m_videoSavePath);
}

void VideoProcessor::setThumbnailSavePath(const QString &path)
{
    m_thumbnailSavePath = path;
    QDir dir;
    dir.mkpath(m_thumbnailSavePath);
}

void VideoProcessor::setThumbnailSize(const QSize &size)
{
    m_thumbnailSize = size;
}

void VideoProcessor::setMaxThreadCount(int count)
{
    m_threadPool.setMaxThreadCount(qMax(1, count));
}

void VideoProcessor::cancelAllTasks()
{
    m_threadPool.clear();
}
