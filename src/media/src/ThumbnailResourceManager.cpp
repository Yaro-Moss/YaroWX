#include "ThumbnailResourceManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QFileInfo>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QStyle>

ThumbnailResourceManager* ThumbnailResourceManager::instance()
{
    static ThumbnailResourceManager instance;
    return &instance;
}

ThumbnailResourceManager::ThumbnailResourceManager(QObject* parent)
    : QObject(parent)
    , m_cache(200*1024)
{
    int idealThreadCount = QThread::idealThreadCount();
    m_threadPool.setMaxThreadCount(qMax(2, idealThreadCount));


    QTimer* cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, [this]() {
        cleanupOldResources(600000);
    });
    cleanupTimer->start(600000);
}

ThumbnailResourceManager::~ThumbnailResourceManager()
{
    m_threadPool.waitForDone();
    clearCache();
}


QPixmap ThumbnailResourceManager::getThumbnail(const QString& resourcePath, const QSize& size,
                                       MediaType type, int radius, const QString& iconPath, QString name)
{
    if(resourcePath.isEmpty()&&type!=MediaType::Avatar) {
        return QPixmap();
    }

    QString cacheKey = generateCacheKey(resourcePath, size, type, radius, iconPath, name);

    QMutexLocker locker(&m_mutex);

    // 查找缓存
    MediaCacheItem* cachedItem = m_cache.object(cacheKey);
    if(cachedItem) {
        cachedItem->lastAccessTime = QDateTime::currentMSecsSinceEpoch();
        cachedItem->accessCount++;
        return cachedItem->media;
    }

    // 不在缓存中，且没有正在加载，启动异步加载
    if(!m_loadingMap.contains(cacheKey)) {
        m_loadingMap[cacheKey] = true;

        // 启动异步加载
        ThumbnailLoadTask* task = new ThumbnailLoadTask(resourcePath, size, type,
                                                        radius, cacheKey, this, iconPath, name);

        m_threadPool.start(task);
    }

    if(type!=MediaType::Avatar) return getPixmap(size);

    return QPixmap();
}

QPixmap ThumbnailResourceManager::getPixmap(const QSize &size)
{
    QPixmap pixmap(100,100);
    pixmap.fill(QColor(255, 255, 255)); // 浅灰色背景

    QPainter painter(&pixmap);
    painter.setPen(Qt::darkGray);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "加载中...");

    return pixmap;
}

void ThumbnailResourceManager::preloadThumbnail(const QString& resourcePath, const QSize& size,
                                        MediaType type, int radius, const QString& iconPath)
{
    if(resourcePath.isEmpty()) return;

    QString cacheKey = generateCacheKey(resourcePath, size, type, radius, iconPath);

    QMutexLocker locker(&m_mutex);

    if(m_cache.contains(cacheKey) || m_loadingMap.contains(cacheKey)) {
        return;
    }

    // 检查文件是否存在
    QFileInfo fileInfo(type == MediaType::ImageThumb || type == MediaType::VideoThumb ? iconPath : resourcePath);
    if(!fileInfo.exists() || !fileInfo.isReadable()) {
        return;
    }

    m_loadingMap[cacheKey] = true;
    ThumbnailLoadTask* task = new ThumbnailLoadTask(resourcePath, size, type, radius, cacheKey, this, iconPath);
    m_threadPool.start(task);
}

// 处理原图片
QPixmap ThumbnailResourceManager::processOriginalImage(const QString &resourcePath)
{
    QImage image(resourcePath);
    return QPixmap::fromImage(image);
}

QPixmap ThumbnailResourceManager::createDefaultThumbnail(const QSize& size, MediaType type, const QString& text)
{
    QPixmap pixmap(size);
    pixmap.fill(QColor(50, 50, 50)); // 灰色背景

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    if (type == MediaType::VideoThumb) {
        addPlayButton(pixmap);
    } else if (type == MediaType::ImageThumb) {
        QString displayText = text.isEmpty() ? "图片" : text;
        addTextIndicator(pixmap, displayText);
    }

    return pixmap;
}

void ThumbnailResourceManager::addPlayButton(QPixmap& pixmap)
{
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制播放三角形
    int triangleSize = qMin(pixmap.width(), pixmap.height()) / 4;
    QPointF center(pixmap.width() / 2.0, pixmap.height() / 2.0);

    QPolygonF triangle;
    triangle << QPointF(center.x() - triangleSize/2, center.y() - triangleSize/2)
             << QPointF(center.x() - triangleSize/2, center.y() + triangleSize/2)
             << QPointF(center.x() + triangleSize/2, center.y());

    painter.setBrush(QColor(255, 255, 255, 255));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(triangle);
}

void ThumbnailResourceManager::addTextIndicator(QPixmap& pixmap, const QString& text)
{
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);

    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
}

QPixmap ThumbnailResourceManager::processThumbnail(const QString &resourcePath, const QSize& size,
                                                   MediaType type, int radius,
                                                   const QString &iconPath, const QString name)
{
    switch(type) {
    case MediaType::Avatar:
        return processAvatar(resourcePath, size, radius, name);
    case MediaType::ImageThumb:
        return processImageThumb(resourcePath, size, iconPath);
    case MediaType::VideoThumb:
        return processVideoThumb(resourcePath, size, iconPath);
    case MediaType::OriginalImage:
        return processOriginalImage(resourcePath);
    default:
        return QPixmap();
    }
}


QPixmap ThumbnailResourceManager::processAvatar(const QString &resourcePath, const QSize& size,
                                                int radius, const QString name)
{
    QImage image(resourcePath);

    if(image.isNull()) {
        // 加载图片失败，生成默认头像
        QPixmap defaultAvatar(size);
        defaultAvatar.fill(Qt::transparent);

        QPainter painter(&defaultAvatar);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 绘制圆角矩形背景
        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, size.width(), size.height()), radius, radius);

        // 根据名字生成个性化背景色
        QColor bgColor;
        if (name.isEmpty()) {
            bgColor = QColor(210, 210, 210);
        } else {
            uint hash = qHash(name);
            int hue = hash % 360;
            int saturation = 80;
            int lightness = 70;
            bgColor = QColor::fromHsl(hue, saturation, lightness);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawPath(path);

        QString displayText = name.isEmpty() ? "U" : name.left(1).toUpper();

        // 绘制名字首字母
        QFont font = painter.font();
        font.setBold(true);

        // 动态计算字体大小
        int fontSize = qMin(size.width(), size.height()) * 0.5;
        fontSize = qMax(fontSize, 12);
        font.setPixelSize(fontSize);

        painter.setFont(font);
        painter.setPen(QColor(255, 255, 255)); // 白色文字

        // 绘制文字
        painter.drawText(QRect(0, 0, size.width(), size.height()),
                         Qt::AlignCenter | Qt::TextSingleLine,
                         displayText);
        return defaultAvatar;
    } else {
        // 图片加载成功，处理原图
        QImage scaledImage = image.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        QPixmap result(size);
        result.fill(Qt::transparent);

        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, size.width(), size.height()), radius, radius);
        painter.setClipPath(path);

        int x = (scaledImage.width() - size.width()) / 2;
        int y = (scaledImage.height() - size.height()) / 2;
        painter.drawImage(0, 0, scaledImage, x, y, size.width(), size.height());

        return result;
    }
}

QPixmap ThumbnailResourceManager::processImageThumb(const QString &resourcePath, const QSize& size,
                                                    const QString &iconPath)
{
    // 如果原图片不存在，返回过去警告缩略图
    if(!QFileInfo::exists(resourcePath)){
        return getWarningThumbnail(iconPath, "image");
    }

    QImage image(iconPath);
    // 在原图存在但缩略图不存在时返回默认缩略图
    if(image.isNull()){
        return createDefaultThumbnail(size, MediaType::ImageThumb);
    }

    QPixmap result = QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    return result;
}

QPixmap ThumbnailResourceManager::processVideoThumb(const QString &resourcePath, const QSize& size,
                                                    const QString &iconPath)
{
    // 如果原视频不存在，则返回过期警告缩略图
    if(!QFileInfo::exists(resourcePath)){
        return getWarningThumbnail(iconPath, "video");
    }

    QImage image(iconPath);
    // 在原视频存在但缩略图不存在时返回默认缩略图
    if(image.isNull()){
        return createDefaultThumbnail(size, MediaType::VideoThumb);
    }

    QPixmap result = QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    addPlayButton(result);    // 添加播放按钮

    return result;
}

QString ThumbnailResourceManager::generateCacheKey(const QString& path,
                                                   const QSize& size,
                                                   MediaType type,
                                                   int radius,
                                                   const QString& iconPath,
                                                   const QString& name) const
{
    QString baseKey = QString("%1_%2_%3_%4")
    .arg(path)
    .arg(static_cast<int>(type))
    .arg(radius)
    .arg(name);

    // 如果size为空，表示原图，在缓存键中特殊标记
    if (size.isEmpty()) {
        baseKey += "_original";
    } else {
        baseKey += QString("_%1x%2").arg(size.width()).arg(size.height());
    }

    if (type == MediaType::ImageThumb || type == MediaType::VideoThumb) {
        baseKey += "_" + iconPath;
    }

    return baseKey;
}

// 估算缓存成本
int ThumbnailResourceManager::estimateCacheCost(const QPixmap& pixmap) const noexcept {
    // 定义常量，提升可维护性
    static constexpr int MIN_CACHE_COST = 1;
    static constexpr int BYTES_PER_KB = 1024;

    // 空图片/无效尺寸直接返回最小成本
    if (pixmap.isNull() || pixmap.width() <= 0 || pixmap.height() <= 0) {
        return MIN_CACHE_COST;
    }

    // QPixmap转QImage，通过QImage获取实际字节数
    const QImage image = pixmap.toImage();
    const qint64 total_bytes = image.sizeInBytes();

    // 转换为KB，确保最小成本为1（64位计算避免溢出）
    const int cost_kb = static_cast<int>(qMax<qint64>(MIN_CACHE_COST, total_bytes / BYTES_PER_KB));

    return cost_kb;
}

void ThumbnailResourceManager::onMediaLoaded(const QString& cacheKey, const QString& resourcePath,
                                         const QPixmap& media, MediaType type, bool success)
{
    {
        QMutexLocker locker(&m_mutex);
        m_loadingMap.remove(cacheKey);
        if(success && !media.isNull()) {
            MediaCacheItem* item = new MediaCacheItem{
                media,
                QDateTime::currentMSecsSinceEpoch(),
                1,
                type,
                estimateCacheCost(media)  // 使用估算的成本
            };
            m_cache.insert(cacheKey, item, item->fileSize);
        }
    }

    if(success && !media.isNull()) {
        emit mediaLoaded(resourcePath, media, type);
    } else {
        emit mediaLoadFailed(resourcePath, type);
    }
}

void ThumbnailResourceManager::cancelLoading(const QString& cacheKey)
{
    QMutexLocker locker(&m_mutex);
    m_loadingMap.remove(cacheKey);
}

void ThumbnailResourceManager::clearCache()
{
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
}

void ThumbnailResourceManager::setCacheSize(int maxSize)
{
    QMutexLocker locker(&m_mutex);
    m_cache.setMaxCost(maxSize);
}

void ThumbnailResourceManager::cleanupOldResources(qint64 maxAgeMs)
{
    QMutexLocker locker(&m_mutex);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<QString> keysToRemove;

    const QList<QString>& keys = m_cache.keys();
    for (const QString& key : keys) {
        MediaCacheItem* item = m_cache.object(key);
        if (item && (now - item->lastAccessTime > maxAgeMs)) {
            keysToRemove.append(key);
        }
    }

    for(const QString& key : keysToRemove) {
        m_cache.remove(key);
    }
}

// 创建警告缩略图
QPixmap ThumbnailResourceManager::getWarningThumbnail(const QString& thumbnailPath,
                                                      const QString &mediaType, const QSize &size)
{
    if(!QFileInfo::exists(thumbnailPath)){
        return createDefaultExpiredThumbnail({100,100}, mediaType);
    }
    return  createExpiredThumbnail(QPixmap(thumbnailPath), mediaType, size);
}

QPixmap ThumbnailResourceManager::createDefaultExpiredThumbnail(const QSize& size,
                                                                const QString& mediaType)
{
    QPixmap resultPixmap(size);
    resultPixmap.fill(QColor(80, 80, 80));

    QPainter painter(&resultPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制半透明遮罩
    painter.fillRect(resultPixmap.rect(), QColor(0, 0, 0, 160));

    // 计算图标大小和位置
    int iconSize = qMin(size.width(), size.height()) / 3;
    QRect iconRect((size.width() - iconSize) / 2,
                   (size.height() - iconSize) / 3,
                   iconSize, iconSize);

    // 绘制警告图标
    QStyle* style = QApplication::style();
    QIcon warningIcon = style->standardIcon(QStyle::SP_MessageBoxWarning);
    warningIcon.paint(&painter, iconRect);

    // 绘制文字
    QFont font = painter.font();
    font.setPointSize(qMax(10, iconSize / 5));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::red);

    QString text = QString("%1已过期或被清除").arg(mediaType=="video"? "视频":"图片");
    QRect textRect(0, iconRect.bottom() + 10,
                   size.width(), iconSize / 2);

    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    painter.end();

    return resultPixmap;
}

QPixmap ThumbnailResourceManager::createExpiredThumbnail(const QPixmap& baseThumbnail,
                                                         const QString& mediaType, const QSize size)
{
    if(baseThumbnail.isNull())return QPixmap();
    QPixmap resultPixmap = baseThumbnail.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter painter(&resultPixmap);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // 绘制半透明遮罩
    painter.fillRect(resultPixmap.rect(), QColor(0, 0, 0, 160));

    // 计算图标大小和位置
    int iconSize = qMin(resultPixmap.width(), resultPixmap.height()) / 3;
    QRect iconRect((resultPixmap.width() - iconSize) / 2,
                   (resultPixmap.height() - iconSize) / 3,
                   iconSize, iconSize);

    // 绘制警告图标
    QStyle* style = QApplication::style();
    QIcon warningIcon = style->standardIcon(QStyle::SP_MessageBoxWarning);
    warningIcon.paint(&painter, iconRect);

    // 绘制文字
    QFont font = painter.font();
    font.setPointSize(qMax(10, iconSize / 5));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);

    QString text = QString("%1已过期或被清除").arg(mediaType=="video"? "视频":"图片");
    QRect textRect(0, iconRect.bottom() + 10,
                   resultPixmap.width(), iconSize / 2);

    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    painter.end();

    return resultPixmap;
}
