#include "LocalMomentController.h"
#include "LocalMomentTable.h"
#include "UserTable.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

LocalMomentController::LocalMomentController(DatabaseManager* dbManager, QObject* parent)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_localMomentTable(nullptr)
    , m_reqIdCounter(0)
    , m_loading(false)
    , m_currentOffset(0)
    , m_hasMore(true)
{
    if (m_dbManager) {
        // 从DatabaseManager获取LocalMomentTable
        m_localMomentTable = m_dbManager->localMomentTable();
        connectSignals();

        // 异步加载当前用户
        if (auto userTable = m_dbManager->userTable()) {
            int reqId = generateReqId();
            QMetaObject::invokeMethod(userTable, "getCurrentUser",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, reqId));
            connect(userTable, &UserTable::currentUserLoaded, this, &LocalMomentController::setCurrentUser);
        }
    } else {
        qWarning() << "LocalMomentController: DatabaseManager is null";
    }
}

LocalMomentController::~LocalMomentController() = default;

// ------------------------------ 辅助方法 ------------------------------
int LocalMomentController::generateReqId()
{
    return m_reqIdCounter.fetchAndAddAcquire(1);
}

void LocalMomentController::connectSignals()
{
    if (!m_localMomentTable) {
        qWarning() << "LocalMomentTable is null, cannot connect signals";
        return;
    }

    // 连接LocalMomentTable信号
    connect(m_localMomentTable, &LocalMomentTable::momentSaved, this, &LocalMomentController::onMomentSaved);
    connect(m_localMomentTable, &LocalMomentTable::momentsLoaded, this, &LocalMomentController::onMomentsLoaded);
    connect(m_localMomentTable, &LocalMomentTable::momentInteractLoaded, this, &LocalMomentController::onMomentInteractLoaded);
    connect(m_localMomentTable, &LocalMomentTable::expiredMomentsCleared, this, &LocalMomentController::onExpiredMomentsCleared);
    connect(m_localMomentTable, &LocalMomentTable::dbError, this, &LocalMomentController::onDbError);
}

qint64 LocalMomentController::generateMomentId()
{
    // 生成唯一ID：时间戳 + 随机数
    return QDateTime::currentMSecsSinceEpoch() + rand() % 1000;
}

LocalMoment LocalMomentController::createMoment(const QString& content,
                                                const QVector<MomentImageInfo>& images,
                                                const MomentImageInfo& video,
                                                int privacyType)
{
    LocalMoment moment;
    moment.momentId = generateMomentId();
    moment.userId = m_currentUser.userId;
    moment.username = m_currentUser.nickname;
    moment.avatarLocalPath = m_currentUser.avatarLocalPath;
    moment.avatarUrl = m_currentUser.avatar;
    moment.content = content;

    // 视频设置（互斥：有视频则图片为空）
    if (!video.url.isEmpty()||!video.localPath.isEmpty()) {
        moment.videoUrl = video.url;
        moment.videoLocalPath = video.localPath;
        moment.videoDownloadStatus = video.downloadStatus;
    } else {
        // 图片设置
        moment.images = images;
    }

    // 权限与缓存
    moment.privacyType = privacyType;
    moment.isDeleted = false;
    moment.syncStatus = 1; // 待同步到服务端
    // 设置过期时间：30天后
    moment.expireTime = QDateTime::currentSecsSinceEpoch() + 30 * 24 * 3600;
    moment.createTime = QDateTime::currentSecsSinceEpoch();
    moment.localUpdateTime = moment.createTime;

    return moment;
}

// ------------------------------ 公开方法 ------------------------------
void LocalMomentController::setCurrentUser(int reqId, const User& user)
{
    if (user.userId != -1) {
        m_currentUser = user;
        // 用户切换后重新加载朋友圈
        loadRecentMoments();
    }
}

void LocalMomentController::publishMoment(const QString& content,
                                          const QVector<MomentImageInfo>& images,
                                          const MomentImageInfo& video,
                                          int privacyType)
{
    if (!m_localMomentTable || m_currentUser.userId == 0) {
        emit momentPublished(false, "Database not ready or user not logged in");
        return;
    }

    // 创建朋友圈对象
    LocalMoment moment = createMoment(content, images, video, privacyType);

    // 异步保存到数据库
    int reqId = generateReqId();
    m_pendingOperations.insert(reqId, "publishMoment");

    QMetaObject::invokeMethod(m_localMomentTable, "saveMoment",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(const LocalMoment&, moment));
}

void LocalMomentController::loadRecentMoments(int limit)
{
    if (!m_localMomentTable || m_loading) return;

    m_loading = true;
    m_currentOffset = 0;
    m_limit = limit;

    int reqId = generateReqId();
    m_pendingOperations.insert(reqId, "loadRecentMoments");

    QMetaObject::invokeMethod(m_localMomentTable, "getMoments",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(int, limit),
                              Q_ARG(int, m_currentOffset));
}

void LocalMomentController::loadMoreMoments(int limit)
{
    if (!m_localMomentTable || m_loading || !m_hasMore) return;

    m_loading = true;
    m_limit = limit;

    int reqId = generateReqId();
    m_pendingOperations.insert(reqId, "loadMoreMoments");

    QMetaObject::invokeMethod(m_localMomentTable, "getMoments",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(int, limit),
                              Q_ARG(int, m_currentOffset));
}

void LocalMomentController::loadMomentInteract(qint64 momentId)
{
    if (!m_localMomentTable) return;

    int reqId = generateReqId();
    m_pendingOperations.insert(reqId, QString("loadInteract_%1").arg(momentId));

    QMetaObject::invokeMethod(m_localMomentTable, "getMomentInteract",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(qint64, momentId));
}

void LocalMomentController::likeMoment(qint64 momentId, bool isLike)
{
    if (!m_localMomentTable || m_currentUser.userId == 0) return;

    // 先获取当前互动数据
    int reqId = generateReqId();
    m_pendingOperations.insert(
        reqId,
        QString("likeMoment_%1_%2").arg(momentId).arg(isLike)
        );
    QMetaObject::invokeMethod(m_localMomentTable, "getMomentInteract",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(qint64, momentId));
}

void LocalMomentController::addComment(qint64 momentId, const MomentCommentInfo& comment)
{
    if (!m_localMomentTable
        || m_currentUser.userId == 0
        || momentId < 0
        || comment.commentId < 0
        || (comment.content.isEmpty()&&comment.image.localPath.isEmpty())) {

        qWarning() << "addComment failed: invalid params (momentId:" << momentId
                   << ", commentId:" << comment.commentId << ")";
        emit commentAdded(momentId, false, "Invalid comment params (empty content or invalid ID)");
        return;
    }

    int reqId = generateReqId();
    QString opKey = QString("addComment_%1").arg(momentId);
    m_pendingOperations.insert(reqId, opKey);
    // 缓存上层传入的**完整评论数据**（包含content/commentId/时间等）
    m_pendingAddComments[momentId] = comment;

    QMetaObject::invokeMethod(m_localMomentTable, "getMomentInteract",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(qint64, momentId));
}

void LocalMomentController::deleteComment(qint64 momentId, qint64 commentId)
{
    if (!m_localMomentTable || m_currentUser.userId == 0 || momentId <= 0 || commentId <= 0) {
        qWarning() << "deleteComment failed: invalid params (momentId:" << momentId << ", commentId:" << commentId << ")";
        return;
    }

    int reqId = generateReqId();
    m_pendingOperations.insert(reqId, QString("deleteComment_%1_%2").arg(momentId).arg(commentId));

    QMetaObject::invokeMethod(m_localMomentTable, "getMomentInteract",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(qint64, momentId));
}

void LocalMomentController::updateMediaDownloadStatus(qint64 momentId, bool isVideo, const QString& mediaUrl, const QString& localPath, int status)
{
    if (!m_localMomentTable) {
        emit mediaDownloadStatusUpdated(false, "Database not ready");
        return;
    }

    int reqId = generateReqId();
    m_pendingOperations.insert(reqId, "updateMediaStatus");

    QMetaObject::invokeMethod(m_localMomentTable, "updateMediaDownloadStatus",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId),
                              Q_ARG(qint64, momentId),
                              Q_ARG(bool, isVideo),
                              Q_ARG(const QString&, mediaUrl),
                              Q_ARG(const QString&, localPath),
                              Q_ARG(int, status));
}

void LocalMomentController::clearExpiredMoments()
{
    if (!m_localMomentTable) return;

    int reqId = generateReqId();
    m_pendingOperations.insert(reqId, "clearExpiredMoments");

    QMetaObject::invokeMethod(m_localMomentTable, "clearExpiredMoments",
                              Qt::QueuedConnection,
                              Q_ARG(int, reqId));
}

// ------------------------------ 槽函数 ------------------------------
void LocalMomentController::onMomentSaved(int reqId, bool ok, const QString& reason)
{
    // 从待处理操作中取出标识，若不存在直接返回（避免空操作）
    if (!m_pendingOperations.contains(reqId)) {
        qWarning() << "onMomentSaved: reqId" << reqId << "not found in pendingOperations";
        return;
    }
    QString operation = m_pendingOperations.take(reqId);

    if (operation == "publishMoment") {
        emit momentPublished(ok, reason);
        if (ok) {
            loadRecentMoments(); // 发布成功后刷新列表
        }
    }
    else if (operation == "updateMediaStatus") {
        emit mediaDownloadStatusUpdated(ok, reason);
    }
    else if (operation.startsWith("likeMoment_")) {
        QStringList opParts = operation.split("_");
        if (opParts.size() < 3) {
            qWarning() << "onMomentSaved likeMoment: invalid op key" << operation;
            return;
        }
        qint64 momentId = opParts[1].toLongLong();
        bool isLike = (opParts[2].toInt()!=0);
        // 校验momentId有效性
        if (momentId <= 0) {
            qWarning() << "onMomentSaved likeMoment: invalid momentId" << momentId;
            return;
        }
        emit likeStatusChanged(momentId, ok, isLike);
        loadMomentInteract(momentId);

    }
    else if (operation.startsWith("addComment_")) {
        QStringList opParts = operation.split("_");
        if (opParts.size() < 2) {
            qWarning() << "onMomentSaved addComment: invalid op key" << operation;
            return;
        }
        qint64 momentId = opParts[1].toLongLong();
        if (momentId <= 0) {
            qWarning() << "onMomentSaved addComment: invalid momentId" << momentId;
            return;
        }
        emit commentAdded(momentId, ok, reason);
    }
    else if (operation.startsWith("deleteComment_")) {
        QStringList parts = operation.split("_");
        if (parts.size() < 3) {
            qWarning() << "onMomentSaved deleteComment: invalid op key" << operation;
            return;
        }
        qint64 momentId = parts[1].toLongLong();
        qint64 commentId = parts[2].toLongLong();
        // 双ID有效性校验
        if (momentId <= 0 || commentId <= 0) {
            qWarning() << "onMomentSaved deleteComment: invalid ID (mId:" << momentId << ", cId:" << commentId << ")";
            return;
        }
        emit commentDeleted(momentId, commentId, ok, reason);
    } else {
        // 处理未定义的操作标识，避免无响应，方便排查问题
        qWarning() << "onMomentSaved: unknown operation type" << operation << "reqId:" << reqId;
    }
}

void LocalMomentController::onMomentsLoaded(int reqId, const QVector<LocalMoment>& moments)
{
    QString operation = m_pendingOperations.take(reqId);
    m_loading = false;

    if (operation == "loadRecentMoments") {
        m_moments = moments;
        m_currentOffset = moments.count();
        m_hasMore = moments.count() >= m_limit;

        // 为每条朋友圈加载互动内容
        for (int i = 0; i < m_moments.size(); i++) {
            m_momentsIndexs[m_moments[i].momentId] = i;
            loadMomentInteract(m_moments[i].momentId);
        }

        emit momentsLoaded(m_moments, m_momentsIndexs, m_hasMore);
    } else if (operation == "loadMoreMoments") {
        if (moments.isEmpty()) {
            m_hasMore = false;
            emit momentsLoaded(m_moments, m_momentsIndexs, false);
            return;
        }

        int oldcount = m_moments.count();

        m_moments.append(moments);
        m_currentOffset += moments.count();

        // 为新加载的朋友圈加载互动内容
        for (int i = 0; i < moments.size(); i++) {
            m_momentsIndexs[moments[i].momentId] = oldcount + i;
            loadMomentInteract(moments[i].momentId);
        }

        emit momentsLoaded(m_moments, m_momentsIndexs, m_hasMore);
    }
}

void LocalMomentController::onMomentInteractLoaded(int reqId, const LocalMomentInteract& interact)
{
    QString operation = m_pendingOperations.take(reqId);
    if (operation.startsWith("loadInteract_")) {
        // 找到对应的朋友圈并更新互动内容
        int index = m_momentsIndexs[interact.momentId];
        m_moments[index].interact = interact;
        emit momentUpdated(m_moments[index]);
    }
    else if(operation.startsWith("likeMoment_")) {
        // 处理点赞逻辑
        LocalMomentInteract newInteract = interact;

        // 不管点赞还是取消，都要先清理
        for (int i = 0; i < newInteract.likes.size(); ++i) {
            if (newInteract.likes[i].userId == m_currentUser.userId) {
                newInteract.likes.remove(i); // 取消点赞
                break;
            }
        }

        // 新增点赞
        QStringList opParts = operation.split("_");
        bool isLike = (opParts[2].toInt()!=0);
        if (isLike) {
            MomentLikeInfo like;
            like.userId = m_currentUser.userId;
            like.username = m_currentUser.nickname;
            like.avatarLocalPath = m_currentUser.avatarLocalPath;
            newInteract.likes.append(like);

        }

        // 保存更新后的互动数据
        int saveReqId = generateReqId();
        m_pendingOperations.insert(saveReqId, operation);
        QMetaObject::invokeMethod(m_localMomentTable, "saveMomentInteract",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, saveReqId),
                                  Q_ARG(const LocalMomentInteract&, newInteract));

    } else if (operation.startsWith("addComment_")) {
        // 解析momentId并校验格式
        QStringList opParts = operation.split("_");
        if (opParts.size() < 2) {
            qWarning() << "addComment failed: invalid operation key" << operation;
            return;
        }
        qint64 momentId = opParts[1].toLongLong();
        if (momentId <= 0) {
            qWarning() << "addComment failed: invalid momentId" << momentId;
            return;
        }

        // 从缓存中取出**完整的评论数据**，
        if (!m_pendingAddComments.contains(momentId)) {
            qWarning() << "addComment failed: no pending comment for momentId" << momentId;
            emit commentAdded(momentId, false, "Comment data lost (no cache)");
            return;
        }
        MomentCommentInfo newComment = m_pendingAddComments.take(momentId);

        LocalMomentInteract newInteract = interact;
        newInteract.momentId = momentId; // 绑定momentId
        if (newInteract.momentId <= 0) {
            qWarning() << "addComment failed: invalid momentId" << momentId;
            return;
        }

        // 防重复评论
        bool isCommentExisted = false;
        for (const auto& c : std::as_const(newInteract.comments)) {
            if (c.commentId == newComment.commentId) {
                isCommentExisted = true;
                break;
            }
        }
        if (isCommentExisted) {
            qInfo() << "comment" << newComment.commentId << "already exists for moment" << momentId;
            emit commentAdded(momentId, false, "Duplicate comment");
            return;
        }

        // 追加**缓存的完整评论**到互动列表
        newInteract.comments.append(newComment);

        // 保存修改后的互动数据
        int saveReqId = generateReqId();
        m_pendingOperations.insert(saveReqId, operation);
        QMetaObject::invokeMethod(m_localMomentTable, "saveMomentInteract",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, saveReqId),
                                  Q_ARG(const LocalMomentInteract&, newInteract));
    }else if (operation.startsWith("deleteComment_")) {
        QStringList opParts = operation.split("_");
        if (opParts.size() < 3) {
            qWarning() << "deleteComment failed: invalid operation key" << operation;
            return;
        }
        qint64 momentId = opParts[1].toLongLong();
        qint64 commentId = opParts[2].toLongLong();
        // ID有效性校验
        if (momentId <= 0 || commentId <= 0) {
            qWarning() << "deleteComment failed: invalid ID (momentId:" << momentId << ", commentId:" << commentId << ")";
            return;
        }

        LocalMomentInteract newInteract = interact;
        newInteract.momentId = momentId;

        // 查找并删除指定commentId的评论
        int commentIndex = -1;
        for (int i = 0; i < newInteract.comments.size(); ++i) {
            if (newInteract.comments[i].commentId == commentId) {
                commentIndex = i;
                break;
            }
        }
        // 未找到评论，直接返回
        if (commentIndex == -1) {
            qWarning() << "deleteComment failed: commentId" << commentId << "not found";
            return;
        }
        newInteract.comments.removeAt(commentIndex);

        // 保存删除后的互动数据
        int saveReqId = generateReqId();
        m_pendingOperations.insert(saveReqId, operation);
        QMetaObject::invokeMethod(m_localMomentTable, "saveMomentInteract",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, saveReqId),
                                  Q_ARG(const LocalMomentInteract&, newInteract));
    }
}

void LocalMomentController::onExpiredMomentsCleared(int reqId, int count, const QString& reason)
{
    m_pendingOperations.take(reqId);
    emit expiredMomentsCleared(count, reason);
    if (count > 0) {
        loadRecentMoments(); // 清理后刷新列表
    }
}

void LocalMomentController::onDbError(int reqId, const QString& error)
{
    qWarning() << "LocalMomentController DB error (reqId:" << reqId << "):" << error;
    m_pendingOperations.take(reqId);
    emit dbError(reqId, error);
}
