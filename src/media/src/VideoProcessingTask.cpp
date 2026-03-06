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
#include <QRegularExpression>
#include <QPixmap>
#include <QByteArray>

// 平台相关的 FFmpeg 可执行文件名
#ifdef Q_OS_WIN
#define FFMPEG_EXEC_NAME "ffmpeg.exe"
#else
#define FFMPEG_EXEC_NAME "ffmpeg"
#endif


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


/**
 * @brief 查找环境变量中的 FFmpeg 路径（跨平台）
 * @return FFmpeg 可执行文件的完整路径，若未找到则返回空字符串
 */
QString VideoProcessingTask::findFfmpegInEnvironment()
{
    // 使用系统命令查找可执行文件路径
#ifdef Q_OS_WIN
    QStringList cmdArgs = {"where", "ffmpeg.exe"};
#else
    QStringList cmdArgs = {"which", "ffmpeg"};
#endif

    QProcess findProc;
    findProc.start(cmdArgs.first(), cmdArgs.mid(1));
    if (findProc.waitForFinished(1000) && findProc.exitCode() == 0) {
        QString output = findProc.readAllStandardOutput().trimmed();
        QStringList paths = output.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
        if (!paths.isEmpty()) {
            QString validPath = paths.first();
            if (QFile::exists(validPath)) {
                return validPath;
            }
        }
    }

    // 直接测试 ffmpeg 是否可执行（如果系统能找到，直接返回命令名）
    QProcess testProc;
    testProc.start(FFMPEG_EXEC_NAME, {"-version"});
    if (testProc.waitForFinished(1000) && testProc.exitCode() == 0) {
        return FFMPEG_EXEC_NAME; // 直接返回命令名，系统会从环境变量查找
    }

    return QString();
}

/**
 * @brief 从视频文件中提取指定时间点的缩略图，返回 QPixmap 对象（不保存到磁盘）
 * @param videoPath    源视频文件路径
 * @param thumbnailSize 缩略图的目标尺寸（保持宽高比缩小至该尺寸内）
 * @param errorMsg     可选输出参数，用于返回错误信息
 * @return 生成的缩略图 QPixmap，失败则返回空 QPixmap
 */
QPixmap VideoProcessingTask::generateVideoThumbnail(const QString &videoPath,
                                                    const QSize &thumbnailSize,
                                                    QString *errorMsg)
{
    // 1. 检查源文件是否存在
    if (!QFile::exists(videoPath)) {
        if (errorMsg) *errorMsg = "源视频文件不存在";
        return QPixmap();
    }

    // 2. 查找 FFmpeg 可执行文件
    QString ffmpegPath;
    QString appDir = QCoreApplication::applicationDirPath();

    // 先检查程序运行目录
    QString localFfmpeg = QDir::cleanPath(appDir + "/" + FFMPEG_EXEC_NAME);
    if (QFile::exists(localFfmpeg)) {
        ffmpegPath = localFfmpeg;
    } else {
        // 向上查找（最多3层）
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

    // 若未找到，则尝试环境变量
    if (ffmpegPath.isEmpty()) {
        ffmpegPath = VideoProcessingTask::findFfmpegInEnvironment();
    }

    if (ffmpegPath.isEmpty() || !QFile::exists(ffmpegPath)) {
        if (errorMsg) *errorMsg = "FFmpeg 工具缺失，请检查程序目录或系统环境变量";
        return QPixmap();
    }

    // 3. 构建 FFmpeg 命令参数，输出到 stdout（管道）
    QProcess ffmpegProcess;
    QStringList arguments;
    arguments << "-i" << videoPath               // 输入文件
              << "-ss" << "00:00:02"              // 跳转到第1秒（可根据需要调整）
              << "-vframes" << "1"                 // 只取一帧
              << "-vf" << QString("scale=%1:%2:force_original_aspect_ratio=decrease")
                              .arg(thumbnailSize.width())
                              .arg(thumbnailSize.height())  // 缩放滤镜
              << "-f" << "image2pipe"              // 输出格式为管道图像流
              << "-vcodec" << "mjpeg"               // 使用 MJPEG 编码（便于 Qt 直接加载）
              << "-";                               // 输出到 stdout

    // 4. 启动进程并读取数据
    ffmpegProcess.start(ffmpegPath, arguments, QIODevice::ReadOnly);
    if (!ffmpegProcess.waitForStarted()) {
        if (errorMsg) *errorMsg = "FFmpeg 进程启动失败：" + ffmpegProcess.errorString();
        return QPixmap();
    }

    // 等待进程结束，同时读取 stdout 数据
    QByteArray imageData;
    if (!ffmpegProcess.waitForFinished(30000)) { // 30秒超时
        ffmpegProcess.kill();
        if (errorMsg) *errorMsg = "FFmpeg 处理超时";
        return QPixmap();
    }

    if (ffmpegProcess.exitCode() != 0) {
        QString err = ffmpegProcess.readAllStandardError();
        if (errorMsg) *errorMsg = "FFmpeg 处理失败，错误信息：" + err;
        return QPixmap();
    }

    imageData = ffmpegProcess.readAllStandardOutput();
    if (imageData.isEmpty()) {
        if (errorMsg) *errorMsg = "FFmpeg 未输出图像数据";
        return QPixmap();
    }

    // 5. 将数据加载为 QImage 并转换为 QPixmap
    QImage image;
    if (!image.loadFromData(imageData, "JPEG")) { // 指定格式为 JPEG
        if (errorMsg) *errorMsg = "无法从 FFmpeg 输出解析图像";
        return QPixmap();
    }

    return QPixmap::fromImage(image);
}

bool VideoProcessingTask::generateVideoThumbnail(const QString &videoPath, const QString &thumbnailPath)
{
    // 调用静态函数获取缩略图
    QString errorMsg;
    QPixmap thumb = VideoProcessingTask::generateVideoThumbnail(videoPath, m_thumbnailSize, &errorMsg);
    if (thumb.isNull()) {
        m_errorMessage = errorMsg;
        return false;
    }

    // 保存到文件
    if (!thumb.save(thumbnailPath, "JPEG")) {
        m_errorMessage = "缩略图保存失败";
        return false;
    }

    return true;
}
