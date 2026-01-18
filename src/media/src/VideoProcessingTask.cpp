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
#include <QCoreApplication>
#include <QRegExp>

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

// 查找环境变量中的FFmpeg路径（跨平台）
static QString findFfmpegInEnvironment()
{
    // 使用系统命令查找可执行文件路径
#ifdef Q_OS_WIN
    // Windows使用where命令查找ffmpeg.exe
    QStringList cmdArgs = {"where", "ffmpeg.exe"};
#else
    // Linux/macOS使用which命令查找ffmpeg
    QStringList cmdArgs = {"which", "ffmpeg"};
#endif

    QProcess findProc;
    findProc.start(cmdArgs.first(), cmdArgs.mid(1));
    // 设置1秒超时，避免阻塞
    if (findProc.waitForFinished(1000) && findProc.exitCode() == 0) {
        QString output = findProc.readAllStandardOutput().trimmed();
        // 处理可能的多行输出（Windows where可能返回多个路径）
        QStringList paths = output.split(QRegExp("[\r\n]"), Qt::SkipEmptyParts);
        if (!paths.isEmpty()) {
            QString validPath = paths.first();
            if (QFile::exists(validPath)) {
                return validPath;
            }
        }
    }

    // 直接测试ffmpeg是否可执行（如果系统能找到，直接返回命令名）
#ifdef Q_OS_WIN
    QString ffmpegCmd = "ffmpeg.exe";
#else
    QString ffmpegCmd = "ffmpeg";
#endif

    QProcess testProc;
    testProc.start(ffmpegCmd, {"-version"});
    if (testProc.waitForFinished(1000) && testProc.exitCode() == 0) {
        return ffmpegCmd; // 直接返回命令名，系统会从环境变量查找
    }

    // 环境变量中未找到
    return QString();
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
        // 2. 找不到则向上查找（最多3层）
        QDir dir(appDir);
        for (int i = 0; i < 3; ++i) {
            dir.cdUp();
            QString parentFfmpeg = QDir::cleanPath(dir.absolutePath() + "/" + FFMPEG_EXEC_NAME);
            if (QFile::exists(parentFfmpeg)) {
                ffmpegPath = parentFfmpeg;
                break;
            }
        }
    }

    // ========== 查找环境变量中的FFmpeg ==========
    if (ffmpegPath.isEmpty()) {
        ffmpegPath = findFfmpegInEnvironment();
        if (!ffmpegPath.isEmpty()) {
            qDebug() << "从系统环境变量找到FFmpeg：" << ffmpegPath;
        }
    }

    // 3. 最终检查
    if (ffmpegPath.isEmpty() || !QFile::exists(ffmpegPath)) {
        // 修改提示信息，包含环境变量的查找
        qDebug() << "FFmpeg不存在！已查找路径：程序目录、上层3层目录、系统环境变量";
        m_errorMessage = QString("FFmpeg工具缺失，请检查thirdparty目录是否有对应平台的FFmpeg文件，或确保FFmpeg已加入系统环境变量");
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