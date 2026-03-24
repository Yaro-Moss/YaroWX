#ifndef THUMBNAILRESOURCEMANAGER_H
#define THUMBNAILRESOURCEMANAGER_H

#include <QObject>
#include <QCache>
#include <QMap>
#include <QString>
#include <QPixmap>
#include <QDateTime>
#include <QMutex>
#include <QThreadPool>
#include <QRunnable>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <QFileInfo>
#include <QPointer>

enum class MediaType {
    Avatar,        // 头像
    ImageThumb,    // 图片缩略图
    VideoThumb,    // 视频缩略图
    OriginalImage  // 原图片
};

class ThumbnailResourceManager : public QObject
{
    Q_OBJECT

public:
    ThumbnailResourceManager(QObject* parent = nullptr); // 短期使用，偶尔在图片展示时使用，不用长期存在，用完清理。
    ~ThumbnailResourceManager();

    static ThumbnailResourceManager* instance(); // 全局的资源（设置为静态资源）

    static void cleanup() {
        if (ThumbnailResourceManager* instance = ThumbnailResourceManager::instance()) {
            instance->m_threadPool.waitForDone();            // 等待所有任务完成
        }
    }

    // 获取资源 - 对于ImageThumb和VideoThumb，resourcePath是原路径，iconPath是图标路径
    QPixmap getThumbnail(const QString& resourcePath, const QSize& size = QSize(),
                         MediaType type = MediaType::Avatar, int radius = 5,
                         const QString& iconPath = QString(), QString name = "?");

    // 预加载资源到缓存
    void preloadThumbnail(const QString& resourcePath, const QSize& size = QSize(),
                      MediaType type = MediaType::Avatar, int radius = 5,
                      const QString& iconPath = QString());

    static QPixmap processThumbnail(const QString& resourcePath, const QSize& size = QSize(),
                                    MediaType type = MediaType::Avatar, int radius = 5,
                                    const QString& iconPath = QString(), const QString name = "?");

    // 获取警告缩略图
    static QPixmap getWarningThumbnail(const QString& thumbnailPath,const QString &mediaType, const QSize &size = QSize(200,300));

    void cancelLoading(const QString& cacheKey); // 取消正在进行的加载任务
    void clearCache(); // 清理缓存
    void setCacheSize(int maxSize);
    void cleanupOldResources(qint64 maxAgeMs = 3600000); // 清理长时间未使用的资源

    static void addPlayButton(QPixmap& pixmap);// 添加播放按钮到视频缩略图
    static void addTextIndicator(QPixmap& pixmap, const QString& text);    // 添加文字标识

signals:
    void mediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type);
    void mediaLoadFailed(const QString& resourcePath, MediaType type);



private slots:
    void onMediaLoaded(const QString& cacheKey, const QString& resourcePath,
                       const QPixmap& media, MediaType type, bool success);


private:

    struct MediaCacheItem {
        QPixmap media;
        qint64 lastAccessTime;
        int accessCount;
        MediaType type;
        qint64 fileSize;
    };

    QPixmap getPixmap(const QSize &size);
    static QPixmap processAvatar(const QString &resourcePath, const QSize& size, int radius, const QString name);
    static QPixmap processImageThumb(const QString &resourcePath, const QSize& size, const QString &iconPath);
    static QPixmap processVideoThumb(const QString &resourcePath, const QSize& size, const QString &iconPath);
    static QPixmap processOriginalImage(const QString &resourcePath);    // 处理原图片加载


    // 创建默认缩略图
    static QPixmap createDefaultThumbnail(const QSize& size, MediaType type, const QString& text = QString());
    static QPixmap createDefaultExpiredThumbnail(const QSize& size, const QString& mediaType);
    static QPixmap createExpiredThumbnail(const QPixmap& baseThumbnail, const QString& mediaType, const QSize size = QSize(200,300));

    QString generateCacheKey(const QString& path,
                             const QSize& size,
                             MediaType type,
                             int radius,
                             const QString& iconPath = QString(),
                             const QString &name = QString()) const;

    int estimateCacheCost(const QPixmap& pixmap) const noexcept ;

    QCache<QString, MediaCacheItem> m_cache;
    QMap<QString, bool> m_loadingMap;  // key: cacheKey, value: 是否正在加载
    QMutex m_mutex;
    QThreadPool m_threadPool;
};



class ThumbnailLoadTask : public QRunnable
{
public:
    ThumbnailLoadTask(const QString& path, const QSize& size, MediaType type,
                  int radius, const QString& cacheKey, ThumbnailResourceManager* manager,
                      const QString& iconPath = QString(), const QString &name="?")
        : m_path(path), m_size(size), m_type(type), m_radius(radius),
        m_cacheKey(cacheKey), m_manager(manager), m_iconPath(iconPath), m_name(name) {}

    void run() override {

        bool success = false;
        QPixmap result;

        result = ThumbnailResourceManager::processThumbnail(m_path, m_size, m_type, m_radius, m_iconPath, m_name);
        success = !result.isNull();


        QMetaObject::invokeMethod(m_manager, "onMediaLoaded",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, m_cacheKey),
                                  Q_ARG(QString, m_path),
                                  Q_ARG(QPixmap, result),
                                  Q_ARG(MediaType, m_type),
                                  Q_ARG(bool, success));
    }

private:
    QString m_path;
    QSize m_size;
    MediaType m_type;
    int m_radius;
    QString m_iconPath;
    QString m_name;

    QString m_cacheKey;
    ThumbnailResourceManager* m_manager;
};

#endif // THUMBNAILRESOURCEMANAGER_H
