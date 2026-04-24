#include "FileUploader.h"
#include "ConfigManager.h"
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent/QtConcurrent>
#include <QMimeDatabase>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

QMutex mutex;
FileUploader* FileUploader::m_instance = nullptr;

FileUploader* FileUploader::instance() {
    QMutexLocker locker(&mutex);
    if (!m_instance)
        m_instance = new FileUploader();
    return m_instance;
}

FileUploader::FileUploader(QObject* parent) : QObject(parent), m_cancelled(false), m_chunkSize(5 * 1024 * 1024) {
    m_manager = new QNetworkAccessManager(this);
    m_manager->setTransferTimeout(60000);
}

FileUploader::~FileUploader() {
    if (m_file.isOpen())
        m_file.close();
}

QString FileUploader::getMimeType(const QString& suffix) const {
    static QMimeDatabase db;
    QString mime = db.mimeTypeForFile("dummy." + suffix).name();
    if (mime.isEmpty() || mime == "application/octet-stream") {
        if (suffix == "jpg" || suffix == "jpeg") return "image/jpeg";
        if (suffix == "png") return "image/png";
        if (suffix == "gif") return "image/gif";
        if (suffix == "mp4") return "video/mp4";
        if (suffix == "pdf") return "application/pdf";
        return "application/octet-stream";
    }
    return mime;
}

QString FileUploader::calculateFileHash(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    const qint64 bufferSize = 1024 * 1024;
    QByteArray buffer;
    while (!file.atEnd()) {
        buffer = file.read(bufferSize);
        hash.addData(buffer);
    }
    return QString::fromLatin1(hash.result().toHex());
}

void FileUploader::uploadFile(const QString& filePath,
                              const QString& thumbnailPath,
                              UploadCallback callback,
                              ProgressCallback progressCb) {
    m_filePath = filePath;
    m_thumbnailPath = thumbnailPath;
    m_callback = callback;
    m_progressCb = progressCb;
    m_cancelled = false;
    m_finalUrl.clear();
    m_thumbUrl.clear();

    QFileInfo info(filePath);
    m_fileSize = info.size();
    if (m_fileSize <= 0) {
        if (m_callback)
            m_callback(false, QString(), QString(), "Empty file");
        return;
    }
    m_totalChunks = (m_fileSize + m_chunkSize - 1) / m_chunkSize;

    QtConcurrent::run([this]() {
        m_fileHash = calculateFileHash(m_filePath);
        if (m_fileHash.isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                if (m_callback)
                    m_callback(false, QString(), QString(), "Failed to hash file");
            }, Qt::QueuedConnection);
            return;
        }
        QMetaObject::invokeMethod(this, "queryUploadedChunks", Qt::QueuedConnection);
    });
}

void FileUploader::queryUploadedChunks() {
    QUrl url(ConfigManager::instance()->getUploadUrl() + "/chunk");
    url.setQuery(QString("hash=%1&total_chunks=%2").arg(m_fileHash).arg(m_totalChunks));
    QNetworkRequest req(url);
    auto reply = m_manager->get(req);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (m_callback)
                m_callback(false, QString(), QString(), "Query status failed: " + reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            if (m_callback)
                m_callback(false, QString(), QString(), "Invalid status response");
            return;
        }
        QJsonObject obj = doc.object();
        if (obj.contains("uploaded_chunks")) {
            QJsonArray arr = obj["uploaded_chunks"].toArray();
            m_uploadedChunks.clear();
            for (auto v : arr)
                m_uploadedChunks.insert(v.toInt());
        }
        if (!m_file.isOpen()) {
            m_file.setFileName(m_filePath);
            if (!m_file.open(QIODevice::ReadOnly)) {
                if (m_callback)
                    m_callback(false, QString(), QString(), "Cannot open file");
                return;
            }
        }
        startUpload();
    });
}

void FileUploader::startUpload() {
    m_currentChunkIndex = 0;
    uploadNextChunk();
}

void FileUploader::uploadNextChunk() {
    if (m_cancelled) return;

    while (m_currentChunkIndex < m_totalChunks && m_uploadedChunks.contains(m_currentChunkIndex))
        m_currentChunkIndex++;

    if (m_currentChunkIndex >= m_totalChunks) {
        completeUpload();
        return;
    }

    qint64 offset = (qint64)m_currentChunkIndex * m_chunkSize;
    if (!m_file.seek(offset)) {
        if (m_callback)
            m_callback(false, QString(), QString(), "Seek error");
        return;
    }
    QByteArray chunkData = m_file.read(m_chunkSize);
    if (chunkData.isEmpty() && m_fileSize > 0) {
        if (m_callback)
            m_callback(false, QString(), QString(), "Read chunk error");
        return;
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart hashPart;
    hashPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"hash\"");
    hashPart.setBody(m_fileHash.toUtf8());
    multiPart->append(hashPart);

    QHttpPart indexPart;
    indexPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"index\"");
    indexPart.setBody(QByteArray::number(m_currentChunkIndex));
    multiPart->append(indexPart);

    QHttpPart chunkPart;
    chunkPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QString("form-data; name=\"chunk\"; filename=\"chunk_%1\"").arg(m_currentChunkIndex));
    chunkPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    QBuffer* buffer = new QBuffer;
    buffer->setData(chunkData);
    buffer->open(QIODevice::ReadOnly);
    chunkPart.setBodyDevice(buffer);
    buffer->setParent(multiPart);
    multiPart->append(chunkPart);

    QUrl url(ConfigManager::instance()->getUploadUrl() + "/chunk");
    QNetworkRequest req(url);
    QNetworkReply* reply = m_manager->post(req, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, [this](qint64 sent, qint64 total) {
        if (m_progressCb && total > 0) {
            qint64 uploadedBytes = (qint64)m_currentChunkIndex * m_chunkSize + sent;
            int percent = (int)(100.0 * uploadedBytes / m_fileSize);
            m_progressCb(percent, uploadedBytes, m_fileSize);
        }
    });

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        bool ok = (reply->error() == QNetworkReply::NoError);
        onChunkUploaded(ok);
    });
}

void FileUploader::onChunkUploaded(bool success) {
    if (m_cancelled) return;
    if (!success) {
        if (m_callback)
            m_callback(false, QString(), QString(), "Chunk upload failed");
        return;
    }
    m_uploadedChunks.insert(m_currentChunkIndex);
    m_currentChunkIndex++;
    uploadNextChunk();
}

void FileUploader::completeUpload() {
    m_file.close();

    QJsonObject reqObj;
    reqObj["hash"] = m_fileHash;
    reqObj["total_chunks"] = m_totalChunks;
    reqObj["filename"] = QFileInfo(m_filePath).fileName();

    QNetworkRequest req(QUrl(ConfigManager::instance()->getUploadUrl() + "/complete"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto reply = m_manager->post(req, QJsonDocument(reqObj).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (m_callback)
                m_callback(false, QString(), QString(), "Complete failed: " + reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("url")) {
                m_finalUrl = obj["url"].toString();
                if (!m_thumbnailPath.isEmpty() && QFile::exists(m_thumbnailPath))
                    uploadThumbnail();
                else
                    m_callback(true, m_finalUrl, QString(), QString());
                return;
            }
        }
        if (m_callback)
            m_callback(false, QString(), QString(), "Invalid complete response");
    });
}

void FileUploader::uploadThumbnail() {
    if (m_thumbnailPath.isEmpty()) {
        qWarning() << "Thumbnail path is empty, skip upload";
        m_callback(true, m_finalUrl, QString(), QString());
        return;
    }
    QFile* thumbFile = new QFile(m_thumbnailPath);
    if (!thumbFile->exists()) {
        qWarning() << "Thumbnail file does not exist:" << m_thumbnailPath;
        m_callback(true, m_finalUrl, QString(), QString()); // 或者视为失败
        delete thumbFile;
        return;
    }
    if (!thumbFile->open(QIODevice::ReadOnly)) {
        if (m_callback)
            m_callback(false, m_finalUrl, QString(), "Cannot open thumbnail file");
        delete thumbFile;
        return;
    }
    QFileInfo info(m_thumbnailPath);
    QString mime = getMimeType(info.suffix());

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"thumbnail\"; filename=\"%1\"").arg(info.fileName()));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, mime);
    filePart.setBodyDevice(thumbFile);
    thumbFile->setParent(multiPart);
    multiPart->append(filePart);

    QUrl url(ConfigManager::instance()->getUploadUrl() + "/thumbnail");
    QNetworkRequest req(url);
    QNetworkReply* reply = m_manager->post(req, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        bool ok = (reply->error() == QNetworkReply::NoError);
        QString thumbUrl;
        if (ok) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject())
                thumbUrl = doc.object()["url"].toString();
        }
        onThumbnailUploaded(ok, thumbUrl);
    });
}

void FileUploader::onThumbnailUploaded(bool success, const QString& thumbUrl) {
    if (success)
        m_callback(true, m_finalUrl, thumbUrl, QString());
    else
        m_callback(false, m_finalUrl, QString(), "Thumbnail upload failed");
}