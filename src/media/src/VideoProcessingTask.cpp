#include "VideoProcessingTask.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QImage>
#include <QProcess>
#include <QStandardPaths>

VideoProcessingTask::VideoProcessingTask(const qint64 conversationId,
                                         const QString &sourceVideoPath,
                                         const QString &videoSavePath,
                                         const QString &thumbnailSavePath,
                                         const QSize &thumbnailSize,
                                         int priority)
    : m_conversationId(conversationId)
    , m_sourceVideoPath(sourceVideoPath)
    , m_videoSavePath(videoSavePath)
    , m_thumbnailSavePath(thumbnailSavePath)
    , m_thumbnailSize(thumbnailSize)
    , m_priority(priority)
{
    setAutoDelete(true);
}

void VideoProcessingTask::run()
{
    try {
        // 1. 检查源文件是否存在
        if (!QFile::exists(m_sourceVideoPath)) {
            m_errorMessage = "源视频文件不存在";
            emit finished(m_conversationId, false, m_sourceVideoPath, "", "", m_errorMessage);
            return;
        }

        QFileInfo sourceFileInfo(m_sourceVideoPath);

        // 2. 生成唯一文件名
        QString fileName = generateUniqueFileName(sourceFileInfo);
        QString thumbFileName = QString("%1_thumb.jpg").arg(QFileInfo(fileName).baseName());

        // 3. 保存原视频到目标路径
        QString originalDestPath = m_videoSavePath + "/" + fileName;
        if (!copyVideoFile(m_sourceVideoPath, originalDestPath)) {
            m_errorMessage = "复制原视频失败";
            emit finished(m_conversationId, false, m_sourceVideoPath, "", "", m_errorMessage);
            return;
        }

        // 4. 生成并保存缩略图
        QString thumbnailDestPath = m_thumbnailSavePath + "/" + thumbFileName;
        if (!generateVideoThumbnail(m_sourceVideoPath, thumbnailDestPath)) {
            m_errorMessage = "生成视频缩略图失败";
            QFile::remove(originalDestPath); // 清理已复制的原视频
            emit finished(m_conversationId, false, m_sourceVideoPath, "", "", m_errorMessage);
            return;
        }

        qDebug() << "视频处理成功 - 原视频:" << originalDestPath
                 << "缩略图:" << thumbnailDestPath;

        emit finished(m_conversationId, true, m_sourceVideoPath, originalDestPath, thumbnailDestPath, "");

    } catch (const std::exception &e) {
        m_errorMessage = QString("处理异常: %1").arg(e.what());
        emit finished(m_conversationId, false, m_sourceVideoPath, "", "", m_errorMessage);
    }
}

QString VideoProcessingTask::generateUniqueFileName(const QFileInfo &fileInfo)
{
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString randomSuffix = QString::number(QRandomGenerator::global()->generate() % 10000);
    return QString("%1_%2_%3.%4")
        .arg(fileInfo.baseName(),
             timestamp,
             randomSuffix,
             fileInfo.suffix());
}

bool VideoProcessingTask::copyVideoFile(const QString &sourcePath, const QString &destPath)
{
    // 确保目标目录存在
    QFileInfo destFileInfo(destPath);
    QDir destDir = destFileInfo.dir();
    if (!destDir.exists()) {
        destDir.mkpath(".");
    }

    return QFile::copy(sourcePath, destPath);
}

bool VideoProcessingTask::generateVideoThumbnail(const QString &videoPath, const QString &thumbnailPath)
{
    // ========== 查找FFmpeg路径 ==========
    QString ffmpegPath;
    QString appDir = QCoreApplication::applicationDirPath();
    
    // 1. 先检查程序运行目录
    QString localFfmpeg = QDir::cleanPath(appDir + "/" + FFMPEG_EXEC_NAME);
    if (QFile::exists(localFfmpeg)) {
        ffmpegPath = localFfmpeg;
    } else {
        // 2. 找不到则向上查找
        QDir dir(appDir);
        // 最多向上找3层目录
        for (int i = 0; i < 3; ++i) {
            dir.cdUp();
            QString parentFfmpeg = QDir::cleanPath(dir.absolutePath() + "/" + FFMPEG_EXEC_NAME);
            if (QFile::exists(parentFfmpeg)) {
                ffmpegPath = parentFfmpeg;
                break;
            }
        }
    }

    // 3. 最终检查
    if (ffmpegPath.isEmpty() || !QFile::exists(ffmpegPath)) {
        qDebug() << "FFmpeg不存在！已查找路径：" << localFfmpeg << " 及上层目录";
        m_errorMessage = QString("FFmpeg工具缺失，请检查thirdparty目录是否有对应平台的FFmpeg文件");
        return false;
    }
    qDebug() << "找到FFmpeg路径：" << ffmpegPath;
    // ========== 路径查找结束 ==========

    QProcess ffmpegProcess;
    QStringList arguments;
    arguments << "-i" << videoPath
              << "-ss" << "00:00:01"
              << "-vframes" << "1"
              << "-vf" << QString("scale=%1:%2:force_original_aspect_ratio=decrease")
                              .arg(m_thumbnailSize.width())
                              .arg(m_thumbnailSize.height())
              << "-y"
              << thumbnailPath;

    ffmpegProcess.start(ffmpegPath, arguments);

    // 以下逻辑完全保留
    if (!ffmpegProcess.waitForFinished(30000)) {
        qDebug() << "FFmpeg处理超时或失败:" << ffmpegProcess.errorString();
        return false;
    }

    if (ffmpegProcess.exitCode() != 0) {
        qDebug() << "FFmpeg处理失败，退出码:" << ffmpegProcess.exitCode();
        qDebug() << "错误输出:" << ffmpegProcess.readAllStandardError();
        return false;
    }

    if (!QFile::exists(thumbnailPath)) {
        qDebug() << "缩略图文件未生成";
        return false;
    }

    QImage thumbnail(thumbnailPath);
    if (thumbnail.isNull()) {
        qDebug() << "生成的缩略图无效";
        QFile::remove(thumbnailPath);
        return false;
    }

    return true;
}