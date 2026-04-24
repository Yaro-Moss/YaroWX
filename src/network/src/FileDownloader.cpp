#include "FileDownloader.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QRegularExpression>

FileDownloader::FileDownloader(QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_file(nullptr)
{
}

QString FileDownloader::extractBaseFileName(const QUrl& url) {
    QString path = url.path();
    if (path.isEmpty())
        return "download";

    // 去掉末尾的斜杠
    if (path.endsWith('/'))
        path.chop(1);

    QString fileName = QFileInfo(path).fileName();

    // 去掉查询参数 (?...) 和锚点 (#...)
    int queryPos = fileName.indexOf('?');
    if (queryPos != -1)
        fileName.truncate(queryPos);
    int fragPos = fileName.indexOf('#');
    if (fragPos != -1)
        fileName.truncate(fragPos);

    if (fileName.isEmpty())
        return "download";

    // 替换掉 Windows/Linux 文件名中的非法字符
    const QRegularExpression invalidChars(R"([\\/:*?"<>|])");
    fileName.replace(invalidChars, "_");
    return fileName;
}

QString FileDownloader::generateUniqueFilePath(const QString& directory, const QString& baseFileName) {
    QDir dir(directory);
    if (!dir.exists()) {
        dir.mkpath(".");   // 确保目录存在
    }

    QString name = QFileInfo(baseFileName).baseName();        // 文件名主体
    QString ext  = QFileInfo(baseFileName).completeSuffix();  // 扩展名（含点，如 ".jpg"）
    if (!ext.isEmpty())
        ext = "." + ext;

    QString candidate = baseFileName;
    int counter = 1;
    while (dir.exists(candidate)) {
        candidate = QString("%1 (%2)%3").arg(name).arg(counter).arg(ext);
        ++counter;
    }
    return dir.filePath(candidate);
}

FileDownloader::~FileDownloader()
{
    if (m_reply) m_reply->deleteLater();
    if (m_file) delete m_file;
}

void FileDownloader::download(const QUrl& url, const QString& savePath)
{
    m_savePath = savePath;

    // 创建本地文件
    m_file = new QFile(savePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        emit error("Cannot open file for writing: " + m_file->errorString());
        delete m_file;
        m_file = nullptr;
        return;
    }

    // 发起网络请求
    QNetworkRequest request(url);
    m_reply = m_manager->get(request);

    // 连接信号：分块写入 + 完成处理
    connect(m_reply, &QNetworkReply::readyRead, this, &FileDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &FileDownloader::onFinished);
}

void FileDownloader::onReadyRead()
{
    if (m_file && m_reply) {
        QByteArray chunk = m_reply->readAll();
        if (!chunk.isEmpty()) {
            if (m_file->write(chunk) == -1) {
                // 写入失败，取消下载
                m_reply->abort();
                emit error("Write file error: " + m_file->errorString());
            }
        }
    }
}

void FileDownloader::onFinished()
{
    bool success = false;

    if (m_reply->error() == QNetworkReply::NoError) {
        if (m_file) {
            m_file->flush();
            m_file->close();
            success = true;
        } else {
            emit error("File object is null");
        }
    } else {
        emit error("Download error: " + m_reply->errorString());
    }

    if (m_file) {
        delete m_file;
        m_file = nullptr;
    }

    if (success) {
        emit finished(true, m_savePath);
    } else {
        // 删除不完整的文件
        QFile::remove(m_savePath);
        emit finished(false, m_savePath);
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}