#include "LocalMomentTable.h"
#include "DbConnectionManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

LocalMomentTable::LocalMomentTable(QObject* parent)
    : QObject(parent)
{
}

LocalMomentTable::~LocalMomentTable() = default;

/**
 * @brief 初始化数据库连接
 */
void LocalMomentTable::init()
{
    m_database = DbConnectionManager::connectionForCurrentThread();
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        QString errorText = m_database ? m_database->lastError().text() : "Failed to get database connection";
        emit dbError(-1, QString("Open DB failed: %1").arg(errorText));
        return;
    }
}

/**
 * @brief 保存/更新朋友圈
 */
void LocalMomentTable::saveMoment(int reqId, const LocalMoment& moment)
{
    // 1. 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        emit momentSaved(reqId, false, "Database is not open");
        return;
    }

    // 2. 检查数据有效性
    if (!moment.isValid()) {
        emit momentSaved(reqId, false, "Invalid moment data (momentId is 0)");
        return;
    }

    // 3. 执行SQL（INSERT OR REPLACE 支持更新）
    QSqlQuery query(*m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO local_moment (
            moment_id, user_id, username, avatar_local_path, avatar_url,
            content, video_local_path, video_url, video_download_status,
            images_info, privacy_type, is_deleted, sync_status,
            expire_time, create_time, local_update_time
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    // 绑定参数（原有逻辑不变）
    query.addBindValue(moment.momentId);
    query.addBindValue(moment.userId);
    query.addBindValue(moment.username);
    query.addBindValue(moment.avatarLocalPath);
    query.addBindValue(moment.avatarUrl);
    query.addBindValue(moment.content);
    query.addBindValue(moment.videoLocalPath);
    query.addBindValue(moment.videoUrl);
    query.addBindValue(moment.videoDownloadStatus);
    query.addBindValue(moment.imagesToJson());
    query.addBindValue(moment.privacyType);
    query.addBindValue(moment.isDeleted ? 1 : 0);
    query.addBindValue(moment.syncStatus);
    query.addBindValue(moment.expireTime);
    query.addBindValue(moment.createTime);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());

    // 4. 执行并发射结果
    if (!query.exec()) {
        QString error = QString("Save moment failed: %1").arg(query.lastError().text());
        qWarning() << error;
        emit momentSaved(reqId, false, error);
        return;
    }

    emit momentSaved(reqId, true, QString());
}

/**
 * @brief 删除朋友圈
 */
void LocalMomentTable::deleteMoment(int reqId, qint64 momentId)
{
    // 1. 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        emit momentDeleted(reqId, false, "Database is not open");
        return;
    }

    // 2. 执行软删除SQL
    QSqlQuery query(*m_database);
    query.prepare(R"(
        UPDATE local_moment SET is_deleted = 1, local_update_time = ?
        WHERE moment_id = ?
    )");
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(momentId);

    // 3. 执行并发射结果
    if (!query.exec()) {
        QString error = QString("Delete moment failed: %1").arg(query.lastError().text());
        qWarning() << error;
        emit momentDeleted(reqId, false, error);
        return;
    }

    bool success = query.numRowsAffected() > 0;
    QString reason = success ? QString() : QString("Moment with ID %1 not found").arg(momentId);
    emit momentDeleted(reqId, success, reason);
}

/**
 * @brief 分页获取朋友圈列表
 */
void LocalMomentTable::getMoments(int reqId, int limit, int offset)
{
    QVector<LocalMoment> moments;

    // 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        emit dbError(reqId, "Database is not open");
        emit momentsLoaded(reqId, moments);
        return;
    }

    // 执行查询SQL
    QSqlQuery query(*m_database);
    query.prepare(R"(
        SELECT
            lm.*,
            u.nickname AS latest_username,
            u.avatar_local_path AS latest_avatar
        FROM local_moment lm
        INNER JOIN users u ON lm.user_id = u.user_id
        WHERE lm.is_deleted = 0
        ORDER BY lm.create_time DESC
        LIMIT ? OFFSET ?
    )");
    query.addBindValue(limit);
    query.addBindValue(offset);

    // 执行SQL
    if (!query.exec()) {
        QString error = QString("Query moments failed: %1").arg(query.lastError().text());
        qWarning() << error;
        emit dbError(reqId, error);
        emit momentsLoaded(reqId, moments);
        return;
    }

    // 解析结果
    while (query.next()) {
        LocalMoment moment;
        moment.momentId = query.value("moment_id").toLongLong();
        moment.userId = query.value("user_id").toLongLong();
        moment.username = query.value("latest_username").toString();
        moment.avatarLocalPath = query.value("latest_avatar").toString();
        moment.avatarUrl = query.value("avatar_url").toString();
        moment.content = query.value("content").toString();

        moment.videoLocalPath = query.value("video_local_path").toString();
        moment.videoUrl = query.value("video_url").toString();
        moment.videoDownloadStatus = query.value("video_download_status").toInt();

        QString imagesJson = query.value("images_info").toString();
        moment.imagesFromJson(imagesJson);

        moment.privacyType = query.value("privacy_type").toInt();
        moment.isDeleted = query.value("is_deleted").toInt() == 1;
        moment.syncStatus = query.value("sync_status").toInt();
        moment.expireTime = query.value("expire_time").toLongLong();
        moment.createTime = query.value("create_time").toLongLong();
        moment.localUpdateTime = query.value("local_update_time").toLongLong();

        moments.append(moment);
    }

    emit momentsLoaded(reqId, moments);
}

/**
 * @brief 根据ID获取单条朋友圈
 */
void LocalMomentTable::getMomentById(int reqId, qint64 momentId)
{
    LocalMoment moment;

    // 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        emit dbError(reqId, "Database is not open");
        emit momentLoaded(reqId, moment);
        return;
    }

    // 执行查询SQL
    QSqlQuery query(*m_database);
    query.prepare(R"(
        SELECT * FROM local_moment
        WHERE moment_id = ? AND is_deleted = 0
    )");
    query.addBindValue(momentId);

    // 执行SQL
    if (!query.exec()) {
        QString error = QString("Query moment by ID failed: %1").arg(query.lastError().text());
        qWarning() << error;
        emit dbError(reqId, error);
        emit momentLoaded(reqId, moment);
        return;
    }

    // 解析结果（原有逻辑不变）
    if (query.next()) {
        moment.momentId = query.value("moment_id").toLongLong();
        moment.userId = query.value("user_id").toLongLong();
        moment.username = query.value("username").toString();
        moment.avatarLocalPath = query.value("avatar_local_path").toString();
        moment.avatarUrl = query.value("avatar_url").toString();
        moment.content = query.value("content").toString();

        moment.videoLocalPath = query.value("video_local_path").toString();
        moment.videoUrl = query.value("video_url").toString();
        moment.videoDownloadStatus = query.value("video_download_status").toInt();

        QString imagesJson = query.value("images_info").toString();
        moment.imagesFromJson(imagesJson);

        moment.privacyType = query.value("privacy_type").toInt();
        moment.isDeleted = query.value("is_deleted").toInt() == 1;
        moment.syncStatus = query.value("sync_status").toInt();
        moment.expireTime = query.value("expire_time").toLongLong();
        moment.createTime = query.value("create_time").toLongLong();
        moment.localUpdateTime = query.value("local_update_time").toLongLong();
    }

    emit momentLoaded(reqId, moment);
}

/**
 * @brief 无信号的内部查询函数
 */
bool LocalMomentTable::getMomentByIdInternal(qint64 momentId, LocalMoment& outMoment)
{
    // 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        qWarning() << "[getMomentByIdInternal] Database is not open or invalid";
        return false;
    }

    // 执行查询SQL
    QSqlQuery query(*m_database);
    query.prepare(R"(
        SELECT * FROM local_moment
        WHERE moment_id = ? AND is_deleted = 0
    )");
    query.addBindValue(momentId);

    // 执行SQL
    if (!query.exec()) {
        QString error = QString("[getMomentByIdInternal] Query failed: %1").arg(query.lastError().text());
        qWarning() << error;
        return false;
    }

    // 解析结果
    if (query.next()) {
        outMoment.momentId = query.value("moment_id").toLongLong();
        outMoment.userId = query.value("user_id").toLongLong();
        outMoment.username = query.value("username").toString();
        outMoment.avatarLocalPath = query.value("avatar_local_path").toString();
        outMoment.avatarUrl = query.value("avatar_url").toString();
        outMoment.content = query.value("content").toString();

        outMoment.videoLocalPath = query.value("video_local_path").toString();
        outMoment.videoUrl = query.value("video_url").toString();
        outMoment.videoDownloadStatus = query.value("video_download_status").toInt();

        QString imagesJson = query.value("images_info").toString();
        outMoment.imagesFromJson(imagesJson);

        outMoment.privacyType = query.value("privacy_type").toInt();
        outMoment.isDeleted = query.value("is_deleted").toInt() == 1;
        outMoment.syncStatus = query.value("sync_status").toInt();
        outMoment.expireTime = query.value("expire_time").toLongLong();
        outMoment.createTime = query.value("create_time").toLongLong();
        outMoment.localUpdateTime = query.value("local_update_time").toLongLong();

        return true;
    }

    qWarning() << "[getMomentByIdInternal] Moment ID" << momentId << "not found or deleted";
    return false;
}

/**
 * @brief 获取朋友圈互动数据（点赞+评论）
 */
void LocalMomentTable::getMomentInteract(int reqId, qint64 momentId)
{
    LocalMomentInteract interact;
    interact.momentId = momentId;

    // 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        emit dbError(reqId, "Database is not open");
        emit momentInteractLoaded(reqId, interact);
        return;
    }

    // 执行查询SQL
    QSqlQuery query(*m_database);
    query.prepare(R"(
        SELECT likes, comments, local_update_time
        FROM local_moment_interact
        WHERE moment_id = ?
    )");
    query.addBindValue(momentId);

    // 执行SQL
    if (!query.exec()) {
        QString error = QString("Query moment interact failed: %1").arg(query.lastError().text());
        qWarning() << error;
        emit dbError(reqId, error);
        emit momentInteractLoaded(reqId, interact);
        return;
    }

    // 解析结果
    if (query.next()) {
        QString likesJson = query.value("likes").toString();
        QJsonDocument likeDoc = QJsonDocument::fromJson(likesJson.toUtf8());
        if (likeDoc.isArray()) {
            QJsonArray likeArray = likeDoc.array();
            for (const auto& likeObj : std::as_const(likeArray)) {
                interact.likes.append(MomentLikeInfo::fromJson(likeObj.toObject()));
            }
        }

        QString commentsJson = query.value("comments").toString();
        QJsonDocument commentDoc = QJsonDocument::fromJson(commentsJson.toUtf8());
        if (commentDoc.isArray()) {
            QJsonArray commentArray = commentDoc.array();
            for (const auto& commentObj : std::as_const(commentArray)) {
                interact.comments.append(MomentCommentInfo::fromJson(commentObj.toObject()));
            }
        }

        interact.localUpdateTime = query.value("local_update_time").toLongLong();
    }

    emit momentInteractLoaded(reqId, interact);
}

/**
 * @brief 保存朋友圈互动数据（点赞+评论）
 */
void LocalMomentTable::saveMomentInteract(int reqId, const LocalMomentInteract& interact)
{
    // 1. 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        emit momentSaved(reqId, false, "Database is not open");
        return;
    }

    // 2. 检查数据有效性
    if (!interact.isValid()) {
        emit momentSaved(reqId, false, "Invalid moment interact data (momentId is 0)");
        return;
    }

    // 3. 序列化点赞/评论为JSON字符串
    QJsonObject interactJson;
    QJsonArray likeArray, commentArray;
    for (const auto& like : interact.likes){
        likeArray.append(like.toJson());
    }
    for (const auto& comment : interact.comments) commentArray.append(comment.toJson());
    interactJson["likes"] = likeArray;
    interactJson["comments"] = commentArray;

    QString likesJson = QJsonDocument(likeArray).toJson(QJsonDocument::Compact);
    QString commentsJson = QJsonDocument(commentArray).toJson(QJsonDocument::Compact);

    // 4. 执行SQL
    QSqlQuery query(*m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO local_moment_interact (
            moment_id, likes, comments, local_update_time
        ) VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(interact.momentId);
    query.addBindValue(likesJson);
    query.addBindValue(commentsJson);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());

    // 5. 执行并发射结果
    if (!query.exec()) {
        QString error = QString("Save moment interact failed: %1").arg(query.lastError().text());
        qWarning() << error;
        emit momentSaved(reqId, false, error);
        return;
    }

    emit momentSaved(reqId, true, QString());
}

/**
 * @brief 更新媒体文件（图片/视频）下载状态
 */
void LocalMomentTable::updateMediaDownloadStatus(int reqId, qint64 momentId, bool isVideo, const QString& mediaUrl, const QString& localPath, int status)
{
    // 检查数据库连接
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        emit momentSaved(reqId, false, "Database is not open");
        return;
    }

    QSqlQuery query(*m_database);
    bool execSuccess = false;

    if (isVideo) {
        // 更新视频下载状态
        query.prepare(R"(
            UPDATE local_moment
            SET video_local_path = ?, video_download_status = ?, local_update_time = ?
            WHERE moment_id = ? AND video_url = ?
        )");
        query.addBindValue(localPath);
        query.addBindValue(status);
        query.addBindValue(QDateTime::currentSecsSinceEpoch());
        query.addBindValue(momentId);
        query.addBindValue(mediaUrl);
    } else {
        // 更新图片下载状态
        LocalMoment moment;
        if (!getMomentByIdInternal(momentId, moment)) {
            emit momentSaved(reqId, false, QString("Moment with ID %1 not found").arg(momentId));
            return;
        }

        // 修改图片状态（原有逻辑不变）
        bool updated = false;
        for (auto& img : moment.images) {
            if (img.url == mediaUrl) {
                img.localPath = localPath;
                img.downloadStatus = status;
                updated = true;
                break;
            }
        }

        if (!updated) {
            emit momentSaved(reqId, false, QString("Image with URL %1 not found in moment %2").arg(mediaUrl).arg(momentId));
            return;
        }

        // 保存更新后的图片列表
        query.prepare(R"(
            UPDATE local_moment
            SET images_info = ?, local_update_time = ?
            WHERE moment_id = ?
        )");
        query.addBindValue(moment.imagesToJson());
        query.addBindValue(QDateTime::currentSecsSinceEpoch());
        query.addBindValue(momentId);
    }

    // 执行SQL
    execSuccess = query.exec();
    if (!execSuccess) {
        QString error = QString("Update media status failed: %1").arg(query.lastError().text());
        qWarning() << error;
        emit momentSaved(reqId, false, error);
        return;
    }

    bool success = query.numRowsAffected() > 0;
    QString reason = success ? QString() : QString("No media found to update (momentId: %1, isVideo: %2)").arg(momentId).arg(isVideo);
    emit momentSaved(reqId, success, reason);
}

/**
 * @brief 清理过期朋友圈数据
 */
static QString generateInClausePlaceholders(int count) {
    if (count <= 0) return "";
    return QString("?").repeated(count).replace(QString("??"), QString(", ?"));
}

void LocalMomentTable::clearExpiredMoments(int reqId)
{
    int deletedCount = 0;

    // 检查数据库连接
    if (!m_database || !m_database->isOpen() || !m_database->isValid()) {
        const QString error = "Database is not open or invalid";
        qWarning() << "[clearExpiredMoments] " << error;
        emit expiredMomentsCleared(reqId, deletedCount, error);
        return;
    }

    // 获取当前时间戳
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // 第一步：查询过期的朋友圈ID
    QSqlQuery query(*m_database);
    const QString selectSql = R"(
        SELECT moment_id FROM local_moment
        WHERE expire_time > 0 AND expire_time < ? AND is_deleted = 0
    )";

    query.prepare(selectSql);
    query.addBindValue(now);

    if (!query.exec()) {
        const QString error = QString("Query expired moments failed: %1")
        .arg(query.lastError().text());
        qWarning() << "[clearExpiredMoments] " << error;
        emit expiredMomentsCleared(reqId, deletedCount, error);
        return;
    }

    // 收集过期ID
    QVector<qint64> expiredIds;
    while (query.next()) {
        expiredIds.append(query.value(0).toLongLong());
    }

    if (expiredIds.isEmpty()) {
        qInfo() << "[clearExpiredMoments] No expired moments found";
        emit expiredMomentsCleared(reqId, 0, QString());
        return;
    }

    // 开启数据库事务
    if (!m_database->transaction()) {
        const QString error = "Start transaction failed: " + m_database->lastError().text();
        qWarning() << "[clearExpiredMoments] " << error;
        emit expiredMomentsCleared(reqId, deletedCount, error);
        return;
    }

    // 第二步：软删除过期朋友圈
    const QString updateSql = QString(R"(
        UPDATE local_moment
        SET is_deleted = 1, local_update_time = ?
        WHERE moment_id IN (%1)
    )").arg(generateInClausePlaceholders(expiredIds.size()));

    query.prepare(updateSql);
    query.addBindValue(now);
    for (qint64 momentId : expiredIds) {
        query.addBindValue(momentId);
    }

    if (!query.exec()) {
        m_database->rollback();
        const QString error = QString("Soft delete expired moments failed: %1")
                                  .arg(query.lastError().text());
        qWarning() << "[clearExpiredMoments] " << error;
        emit expiredMomentsCleared(reqId, deletedCount, error);
        return;
    }

    const int updatedCount = query.numRowsAffected();

    // 第三步：删除对应的互动数据
    const QString deleteInteractSql = QString(R"(
        DELETE FROM local_moment_interact
        WHERE moment_id IN (%1)
    )").arg(generateInClausePlaceholders(expiredIds.size()));

    query.prepare(deleteInteractSql);
    for (qint64 momentId : expiredIds) {
        query.addBindValue(momentId);
    }

    if (!query.exec()) {
        m_database->rollback();
        const QString error = QString("Delete expired interact data failed: %1")
                                  .arg(query.lastError().text());
        qWarning() << "[clearExpiredMoments] " << error;
        emit expiredMomentsCleared(reqId, deletedCount, error);
        return;
    }

    // 事务提交
    if (!m_database->commit()) {
        m_database->rollback();
        const QString error = "Commit transaction failed: " + m_database->lastError().text();
        qWarning() << "[clearExpiredMoments] " << error;
        emit expiredMomentsCleared(reqId, deletedCount, error);
        return;
    }

    // 发射结果
    deletedCount = updatedCount;
    qInfo() << "[clearExpiredMoments] Success: deleted" << deletedCount << "expired moments";
    emit expiredMomentsCleared(reqId, deletedCount, QString());
}
