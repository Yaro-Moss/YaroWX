#include "LocalMomentController.h"
#include "ORM.h"
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>

// 生成唯一朋友圈ID
qint64 LocalMomentController::generateMomentId()
{
    return QDateTime::currentMSecsSinceEpoch() + QRandomGenerator::global()->generate() % 1000;
}

// 转换：数据库模型 -> 业务模型
LocalMoment LocalMomentController::fromDbToModel(const LocalMomentDb& dbMoment)
{
    LocalMoment moment;
    moment.momentId = dbMoment.moment_idValue();
    moment.userId = dbMoment.user_idValue();
    moment.username = dbMoment.usernameValue();
    moment.avatarLocalPath = dbMoment.avatar_local_pathValue();
    moment.avatarUrl = dbMoment.avatar_urlValue();
    moment.content = dbMoment.contentValue();
    moment.videoLocalPath = dbMoment.video_local_pathValue();
    moment.videoUrl = dbMoment.video_urlValue();
    moment.videoDownloadStatus = dbMoment.video_download_statusValue();
    moment.imagesFromJson(dbMoment.images_infoValue());
    moment.privacyType = dbMoment.privacy_typeValue();
    moment.isDeleted = dbMoment.is_deletedValue();
    moment.syncStatus = dbMoment.sync_statusValue();
    moment.expireTime = dbMoment.expire_timeValue();
    moment.createTime = dbMoment.create_timeValue();
    moment.localUpdateTime = dbMoment.local_update_timeValue();
    return moment;
}

// 转换：业务模型 -> 数据库模型
LocalMomentDb LocalMomentController::fromModelToDb(const LocalMoment& model)
{
    LocalMomentDb dbMoment;
    dbMoment.setmoment_id(model.momentId);
    dbMoment.setuser_id(model.userId);
    dbMoment.setusername(model.username);
    dbMoment.setavatar_local_path(model.avatarLocalPath);
    dbMoment.setavatar_url(model.avatarUrl);
    dbMoment.setcontent(model.content);
    dbMoment.setvideo_local_path(model.videoLocalPath);
    dbMoment.setvideo_url(model.videoUrl);
    dbMoment.setvideo_download_status(model.videoDownloadStatus);
    dbMoment.setimages_info(model.imagesToJson());
    dbMoment.setprivacy_type(model.privacyType);
    dbMoment.setis_deleted(model.isDeleted ? 1 : 0);
    dbMoment.setsync_status(model.syncStatus);
    dbMoment.setexpire_time(model.expireTime);
    dbMoment.setcreate_time(model.createTime);
    dbMoment.setlocal_update_time(model.localUpdateTime);
    return dbMoment;
}

// 互动模型转数据库模型
LocalMomentInteractDb LocalMomentController::fromInteractModelToDb(const LocalMomentInteract& interact)
{
    LocalMomentInteractDb db;
    db.setinteract_id(interact.interactId);
    db.setmoment_id(interact.momentId);
    db.setlikes(interact.likesToJson());
    db.setcomments(interact.commentsToJson());
    // local_update_time 由数据库默认填充
    return db;
}

// 数据库模型转互动模型
LocalMomentInteract LocalMomentController::fromInteractDbToModel(const LocalMomentInteractDb& db)
{
    return LocalMomentInteract::fromDb(db);
}

// 更新缓存中的朋友圈
void LocalMomentController::updateMomentInCache(const LocalMoment& moment)
{
    for (int i = 0; i < m_moments.size(); ++i) {
        if (m_moments[i].momentId == moment.momentId) {
            m_moments[i] = moment;
            emit momentUpdated(moment);
            break;
        }
    }
}

// 构造函数/析构函数
LocalMomentController::LocalMomentController(QObject* parent)
    : QObject(parent)
{
}

LocalMomentController::~LocalMomentController()
{
}

// 设置当前登录用户
void LocalMomentController::setCurrentLoginUser(const Contact &user)
{
    m_currentLoginUser = user;
}

// 发布朋友圈
void LocalMomentController::publishMoment(const QString& content,
                                          const QVector<MomentImageInfo>& images,
                                          const MomentImageInfo& video,
                                          int privacyType)
{
    LocalMoment moment;
    moment.momentId = generateMomentId();
    moment.userId = m_currentLoginUser.user_idValue();
    moment.username = m_currentLoginUser.user.nicknameValue();
    moment.avatarLocalPath = m_currentLoginUser.user.avatar_local_pathValue();
    moment.avatarUrl = m_currentLoginUser.user.avatarValue();
    moment.content = content;
    if (!video.url.isEmpty() || !video.localPath.isEmpty()) {
        moment.videoUrl = video.url;
        moment.videoLocalPath = video.localPath;
        moment.videoDownloadStatus = video.downloadStatus;
    } else {
        moment.images = images;
    }
    moment.privacyType = privacyType;
    moment.isDeleted = false;
    moment.syncStatus = 1; // 待同步
    moment.expireTime = QDateTime::currentSecsSinceEpoch() + 30 * 24 * 3600;
    moment.createTime = QDateTime::currentSecsSinceEpoch();
    moment.localUpdateTime = moment.createTime;

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, moment]() {
                bool success = watcher->result();
                emit momentPublished(success);
                if (success) {
                    m_moments.insert(0, moment);
                    emit momentsLoaded(m_moments, m_hasMore);
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([this, moment]() -> bool {
        Orm orm;
        LocalMomentDb db = fromModelToDb(moment);
        return orm.insert(db);
    });
    watcher->setFuture(future);
}

// 加载最近朋友圈
void LocalMomentController::loadRecentMoments(int limit)
{
    if (m_loading) return;
    m_loading = true;
    m_currentOffset = 0;
    m_hasMore = true;

    QFutureWatcher<QList<LocalMomentDb>>* watcher = new QFutureWatcher<QList<LocalMomentDb>>(this);
    connect(watcher, &QFutureWatcher<QList<LocalMomentDb>>::finished, this,
            [this, watcher, limit]() {
                QList<LocalMomentDb> dbList = watcher->result();
                QVector<LocalMoment> moments;
                for (const auto& db : dbList) {
                    moments.append(fromDbToModel(db));
                }
                m_moments = moments;
                m_hasMore = (moments.size() == limit);
                m_currentOffset = moments.size();
                emit momentsLoaded(m_moments, m_hasMore);
                m_loading = false;
                watcher->deleteLater();
            });

    QFuture<QList<LocalMomentDb>> future = QtConcurrent::run([limit]() -> QList<LocalMomentDb> {
        Orm orm;
        return orm.findWhere<LocalMomentDb>(
            "is_deleted = 0", {}, "create_time DESC", limit, 0
            );
    });
    watcher->setFuture(future);
}

// 加载更多朋友圈
void LocalMomentController::loadMoreMoments(int limit)
{
    if (m_loading || !m_hasMore) return;
    m_loading = true;

    int offset = m_currentOffset;
    QFutureWatcher<QList<LocalMomentDb>>* watcher = new QFutureWatcher<QList<LocalMomentDb>>(this);
    connect(watcher, &QFutureWatcher<QList<LocalMomentDb>>::finished, this,
            [this, watcher, offset, limit]() {
                QList<LocalMomentDb> dbList = watcher->result();
                if (dbList.isEmpty()) {
                    m_hasMore = false;
                } else {
                    for (const auto& db : std::as_const(dbList)) {
                        m_moments.append(fromDbToModel(db));
                    }
                    m_hasMore = (dbList.size() == limit);
                    m_currentOffset = m_moments.size();
                }
                emit momentsLoaded(m_moments, m_hasMore);
                m_loading = false;
                watcher->deleteLater();
            });

    QFuture<QList<LocalMomentDb>> future = QtConcurrent::run([offset, limit]() -> QList<LocalMomentDb> {
        Orm orm;
        return orm.findWhere<LocalMomentDb>(
            "is_deleted = 0", {}, "create_time DESC", limit, offset
            );
    });
    watcher->setFuture(future);
}

// 加载朋友圈互动信息
void LocalMomentController::loadMomentInteract(qint64 momentId)
{
    QFutureWatcher<QList<LocalMomentInteractDb>>* watcher = new QFutureWatcher<QList<LocalMomentInteractDb>>(this);
    connect(watcher, &QFutureWatcher<QList<LocalMomentInteractDb>>::finished, this,
            [this, momentId, watcher]() {
                QList<LocalMomentInteractDb> list = watcher->result();
                if (!list.isEmpty()) {
                    LocalMomentInteract interact = fromInteractDbToModel(list.first());
                    for (int i = 0; i < m_moments.size(); ++i) {
                        if (m_moments[i].momentId == momentId) {
                            m_moments[i].interact = interact;
                            emit momentUpdated(m_moments[i]);
                            break;
                        }
                    }
                }
                watcher->deleteLater();
            });

    QFuture<QList<LocalMomentInteractDb>> future = QtConcurrent::run([momentId]() -> QList<LocalMomentInteractDb> {
        Orm orm;
        return orm.findWhere<LocalMomentInteractDb>("moment_id = ?", {momentId});
    });
    watcher->setFuture(future);
}

// 点赞/取消点赞
void LocalMomentController::likeMoment(qint64 momentId, bool isLike)
{
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
        [this, watcher, momentId, isLike]() {
            bool success = watcher->result();
            if (success)
                loadMomentInteract(momentId);
            watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([this, momentId, isLike]() -> bool {
        Orm orm;
        auto list = orm.findWhere<LocalMomentInteractDb>("moment_id = ?", {momentId});
        LocalMomentInteractDb db;
        if (!list.isEmpty())
            db = list.first();
        else db.setmoment_id(momentId);

        LocalMomentInteract interact = fromInteractDbToModel(db);
        // 查找当前用户是否已点赞
        auto it = std::find_if(interact.likes.begin(), interact.likes.end(),
                               [this](const MomentLikeInfo& like) {
                                   return like.userId == m_currentLoginUser.user_id();
                               });
        if (isLike && it == interact.likes.end()) {
            MomentLikeInfo like;
            like.userId = m_currentLoginUser.user_idValue();
            like.username = m_currentLoginUser.user.nicknameValue();
            like.avatarLocalPath = m_currentLoginUser.user.avatar_local_pathValue();
            interact.likes.append(like);
        } else if (!isLike && it != interact.likes.end()) {
            interact.likes.erase(it);
        } else {
            return true; // 状态未变
        }

        bool success = false;
        LocalMomentInteractDb newDb = fromInteractModelToDb(interact);
        if (!list.isEmpty())
            success = orm.update(newDb);
        else success = orm.insert(newDb);

        if(!success) qDebug()<<orm.lastError();
        return success;

    });
    watcher->setFuture(future);
}

// 添加评论
void LocalMomentController::addComment(qint64 momentId, const MomentCommentInfo& comment)
{
    if (momentId < 0 || comment.commentId < 0 ||
        (comment.content.isEmpty() && comment.image.localPath.isEmpty())) {
        return;
    }

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, momentId]() {
                bool success = watcher->result();
                if (success) loadMomentInteract(momentId);
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([this, momentId, comment]() -> bool {
        Orm orm;
        auto list = orm.findWhere<LocalMomentInteractDb>("moment_id = ?", {momentId});
        LocalMomentInteractDb db;
        if (!list.isEmpty())
            db = list.first();
        else db.setmoment_id(momentId);

        LocalMomentInteract interact = fromInteractDbToModel(db);
        interact.comments.append(comment);
        LocalMomentInteractDb newDb = fromInteractModelToDb(interact);

        bool success = false;
        if (!list.isEmpty())
            success = orm.update(newDb);
        else success = orm.insert(newDb);

        if(!success) qDebug()<<orm.lastError();
        return success;
    });
    watcher->setFuture(future);
}

// 删除评论
void LocalMomentController::deleteComment(qint64 momentId, qint64 commentId)
{
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, momentId, commentId]() {
                bool success = watcher->result();
                emit commentDeleted(momentId, commentId, success);
                if (success) loadMomentInteract(momentId);
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([this, momentId, commentId]() -> bool {
        Orm orm;
        auto list = orm.findWhere<LocalMomentInteractDb>("moment_id = ?", {momentId});
        if (list.isEmpty()) return false;
        LocalMomentInteract interact = fromInteractDbToModel(list.first());
        bool removed = false;
        for (auto it = interact.comments.begin(); it != interact.comments.end(); ++it) {
            if (it->commentId == commentId) {
                interact.comments.erase(it);
                removed = true;
                break;
            }
        }
        if (!removed) return true; // 未找到，视为成功
        LocalMomentInteractDb newDb = fromInteractModelToDb(interact);
        return orm.update(newDb);
    });
    watcher->setFuture(future);
}

// 更新媒体下载状态
void LocalMomentController::updateMediaDownloadStatus(qint64 momentId, bool isVideo,
                                                      const QString& mediaUrl,
                                                      const QString& localPath, int status)
{
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher]() {
                emit mediaDownloadStatusUpdated(watcher->result());
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([this, momentId, isVideo, mediaUrl, localPath, status]() -> bool {
        Orm orm;
        auto opt = orm.findById<LocalMomentDb>(momentId);
        if (!opt) return false;
        LocalMomentDb db = *opt;
        if (isVideo) {
            db.setvideo_local_path(localPath);
            db.setvideo_download_status(status);
        } else {
            // 更新图片
            LocalMoment moment = fromDbToModel(db);
            for (auto& img : moment.images) {
                if (img.url == mediaUrl) {
                    img.localPath = localPath;
                    img.downloadStatus = status;
                    break;
                }
            }
            db.setimages_info(moment.imagesToJson());
        }
        return orm.update(db);
    });
    watcher->setFuture(future);
}

// 清理过期朋友圈
void LocalMomentController::clearExpiredMoments()
{
    QFutureWatcher<int>* watcher = new QFutureWatcher<int>(this);
    connect(watcher, &QFutureWatcher<int>::finished, this,
            [this, watcher]() {
                int count = watcher->result();
                emit expiredMomentsCleared(count);
                watcher->deleteLater();
            });

    QFuture<int> future = QtConcurrent::run([]() -> int {
        Orm orm;
        qint64 now = QDateTime::currentSecsSinceEpoch();
        auto expired = orm.findWhere<LocalMomentDb>(
            "expire_time < ? AND is_deleted = 0", {now});
        int count = 0;
        for (auto& db : expired) {
            db.setis_deleted(1);
            if (orm.update(db)) ++count;
        }
        return count;
    });
    watcher->setFuture(future);
}
