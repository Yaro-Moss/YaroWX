#pragma once

#include <QObject>
#include <functional>
#include <QtNetwork/QNetworkAccessManager>
#include <QFile>

class QNetworkReply;

class FileUploader : public QObject {
    Q_OBJECT
public:
    using UploadCallback = std::function<void(bool success, const QString& url, const QString& thumbUrl, const QString& errorMsg)>;
    using ProgressCallback = std::function<void(int percent, qint64 uploadedBytes, qint64 totalBytes)>;

    static FileUploader* instance();

    void uploadFile(const QString& filePath,
                    const QString& thumbnailPath,
                    UploadCallback callback,
                    ProgressCallback progressCb = nullptr);

private slots:
    void queryUploadedChunks();


private:
    explicit FileUploader(QObject* parent = nullptr);
    ~FileUploader();

    void startUpload();
    void uploadNextChunk();
    void onChunkUploaded(bool success);
    void completeUpload();
    void uploadThumbnail();
    void onThumbnailUploaded(bool success, const QString& thumbUrl);

    QString calculateFileHash(const QString& filePath);
    QString getMimeType(const QString& suffix) const;

    static FileUploader* m_instance;
    QNetworkAccessManager* m_manager;

    QString m_filePath;
    QString m_thumbnailPath;
    UploadCallback m_callback;
    ProgressCallback m_progressCb;

    QString m_fileHash;
    qint64 m_fileSize;
    int m_chunkSize;
    int m_totalChunks;
    QSet<int> m_uploadedChunks;
    int m_currentChunkIndex;
    QFile m_file;
    bool m_cancelled;

    QString m_finalUrl;
    QString m_thumbUrl;
};