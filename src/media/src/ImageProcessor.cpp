#include "ImageProcessor.h"
#include "ConfigManager.h"
#include "ImageProcessingTask.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QMutexLocker>

ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent)
    , m_thumbnailSize(200, 300)
{
    ConfigManager* configManager = ConfigManager::instance();
    QDir baseDir(configManager->dataSavePath());

    // 设置原图和缩略图保存路径
    m_imageSavePath = baseDir.absoluteFilePath("images");
    m_thumbnailSavePath = baseDir.absoluteFilePath("image_thumbnails");

    // 确保目录存在
    baseDir.mkpath("images");
    baseDir.mkpath("image_thumbnails");

    // 默认最大线程数设置为CPU核心数，避免过多并发
    int idealThreadCount = QThread::idealThreadCount();
    m_threadPool.setMaxThreadCount(qMax(2, idealThreadCount)); // 至少2个线程
}

ImageProcessor::~ImageProcessor()
{
    // 等待所有任务完成
    m_threadPool.waitForDone(5000); // 最多等待5秒
}

void ImageProcessor::processImage(const qint64 conversationId, const QString &sourceImagePath, int priority)
{
    processImages(conversationId, QStringList() << sourceImagePath, priority);
}

void ImageProcessor::processImages(const qint64 conversationId, const QStringList &sourceImagePaths, int priority)
{
    foreach (const QString &imagePath, sourceImagePaths) {

        // 创建处理任务
        ImageProcessingTask *task = new ImageProcessingTask(conversationId,
                                                            imagePath,
                                                            m_imageSavePath,
                                                            m_thumbnailSavePath,
                                                            m_thumbnailSize,
                                                            priority);

        // 连接任务完成信号
        connect(task, &ImageProcessingTask::finished,
                this, &ImageProcessor::onTaskFinished);

        // 提交到线程池
        m_threadPool.start(task);
    }
}

void ImageProcessor::onTaskFinished(const qint64 conversationId,
                                    bool success,
                                    const QString &sourcePath,
                                    const QString &originalPath,
                                    const QString &thumbnailPath,
                                    const QString &errorMessage)
{
    if (success) {
        emit imageProcessed(conversationId, originalPath, thumbnailPath, true);
    } else {
        emit imageProcessingFailed(conversationId, sourcePath, errorMessage);
    }
}

void ImageProcessor::setImageSavePath(const QString &path)
{
    m_imageSavePath = path;
    QDir dir;
    dir.mkpath(m_imageSavePath);
}

void ImageProcessor::setThumbnailSavePath(const QString &path)
{
    m_thumbnailSavePath = path;
    QDir dir;
    dir.mkpath(m_thumbnailSavePath);
}

void ImageProcessor::setThumbnailSize(const QSize &size)
{
    m_thumbnailSize = size;
}

void ImageProcessor::setMaxThreadCount(int count)
{
    m_threadPool.setMaxThreadCount(qMax(1, count));
}

void ImageProcessor::cancelAllTasks()
{
    m_threadPool.clear();
}
