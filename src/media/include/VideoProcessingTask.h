#ifndef VIDEOPROCESSINGTASK_H
#define VIDEOPROCESSINGTASK_H

#include <QRunnable>
#include <QObject>
#include <QString>
#include <QSize>
#include <QFileInfo>


class VideoProcessingTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    VideoProcessingTask(const qint64 conversationId,
                        const QString &sourceVideoPath,
                        const QString &videoSavePath,
                        const QString &thumbnailSavePath,
                        const QSize &thumbnailSize,
                        int priority = 0);

    void run() override;

    static QString findFfmpegInEnvironment();
    static QPixmap generateVideoThumbnail(const QString &videoPath,
                                          const QSize &thumbnailSize,
                                          QString *errorMsg = nullptr);

signals:
    void finished(qint64 conversationId,
                  bool success,
                  const QString &sourcePath,
                  const QString &originalPath,
                  const QString &thumbnailPath,
                  const QString &errorMessage);

private:
    qint64 m_conversationId;
    QString m_sourceVideoPath;
    QString m_videoSavePath;
    QString m_thumbnailSavePath;
    QSize m_thumbnailSize;
    int m_priority;
    QString m_errorMessage;

    QString generateUniqueFileName(const QFileInfo &fileInfo);
    bool copyVideoFile(const QString &sourcePath, const QString &destPath);
    bool generateVideoThumbnail(const QString &videoPath, const QString &thumbnailPath);
};

#endif // VIDEOPROCESSINGTASK_H
