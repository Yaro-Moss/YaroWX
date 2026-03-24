#ifndef LOCALMOMENTCONTROLLER_H
#define LOCALMOMENTCONTROLLER_H

#include <QObject>
#include <QVector>
#include <QHash>
#include <QAtomicInteger>
#include "Contact.h"
#include "LocalMoment.h"

class LocalMomentController : public QObject
{
    Q_OBJECT

public:
    explicit LocalMomentController(QObject* parent = nullptr);
    ~LocalMomentController();

    // 属性
    QVector<LocalMoment> currentMoments() const { return m_moments; }
    qint64 currentLoginUserId() const { return m_currentLoginUser.user.user_idValue(); }
    Contact currentLoginUser() const { return m_currentLoginUser; }

    // 业务操作
    void setCurrentLoginUser(const Contact &user);
    void publishMoment(const QString& content,
                       const QVector<MomentImageInfo>& images = QVector<MomentImageInfo>(),
                       const MomentImageInfo& video = MomentImageInfo(),
                       int privacyType = 0);
    void loadRecentMoments(int limit = 30);
    void loadMoreMoments(int limit = 20);
    void loadMomentInteract(qint64 momentId);
    void likeMoment(qint64 momentId, bool isLike = true);
    void addComment(qint64 momentId, const MomentCommentInfo& comment);
    void deleteComment(qint64 momentId, qint64 commentId);
    void updateMediaDownloadStatus(qint64 momentId, bool isVideo,
                                   const QString& mediaUrl, const QString& localPath,
                                   int status);
    void clearExpiredMoments();

signals:
    void momentPublished(bool success, const QString& error = QString());
    void momentsLoaded(const QVector<LocalMoment>& moments, bool hasMore);
    void momentUpdated(const LocalMoment& moment);
    void commentDeleted(qint64 momentId, qint64 commentId, bool success, const QString& error = QString());
    void mediaDownloadStatusUpdated(bool success, const QString& error = QString());
    void expiredMomentsCleared(int count, const QString& error = QString());

private:
    // 辅助转换方法
    qint64 generateMomentId();
    LocalMoment fromDbToModel(const LocalMomentDb& dbMoment);
    LocalMomentDb fromModelToDb(const LocalMoment& model);
    LocalMomentInteractDb fromInteractModelToDb(const LocalMomentInteract& interact);
    LocalMomentInteract fromInteractDbToModel(const LocalMomentInteractDb& dbInteract);
    void updateMomentInCache(const LocalMoment& moment);

    Contact m_currentLoginUser;
    QVector<LocalMoment> m_moments;
    int m_currentOffset = 0;
    bool m_hasMore = true;
    bool m_loading = false;
};

#endif // LOCALMOMENTCONTROLLER_H
