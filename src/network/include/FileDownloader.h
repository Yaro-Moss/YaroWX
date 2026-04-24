#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

class FileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit FileDownloader(QObject* parent = nullptr);
    ~FileDownloader();

    void download(const QUrl& url, const QString& savePath);
    static QString extractBaseFileName(const QUrl& url);
    static QString generateUniqueFilePath(const QString& directory, const QString& baseFileName);

signals:
    void finished(bool success, const QString& localPath);
    void error(const QString& errorMsg);

private slots:
    void onReadyRead();
    void onFinished();

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_reply;
    QFile* m_file;
    QString m_savePath;
};

#endif // FILEDOWNLOADER_H