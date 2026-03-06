#ifndef LOCALMOMENTCONTROLLER_H
#define LOCALMOMENTCONTROLLER_H

#include <QObject>
#include <QAtomicInteger>
#include <QHash>
#include <QVector>
#include "LocalMoment.h"
#include "DatabaseManager.h"
#include "User.h"

/**
 * @class LocalMomentController
 * @brief 朋友圈控制器，管理朋友圈核心业务逻辑（发布、加载、点赞、评论等）
 */
class LocalMomentController : public QObject
{
    Q_OBJECT

public:
    explicit LocalMomentController(DatabaseManager* dbManager, QObject* parent = nullptr);
    ~LocalMomentController() override;

    // 获取属性
    QVector<LocalMoment> currentMoments() const { return m_moments; }
    qint64 currentUserId() const { return m_currentUser.userId; }
    User currentUser()const{ return m_currentUser;}

    // 核心业务操作
    void setCurrentUser(int reqId, const User& user);
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

    void updateMediaDownloadStatus(qint64 momentId,
                                   bool isVideo,
                                   const QString& mediaUrl,
                                   const QString& localPath,
                                   int status);
    void clearExpiredMoments();

signals:
    // 操作结果信号
    void momentPublished(bool success, const QString& error = QString());
    void momentsLoaded(const QVector<LocalMoment>& moments, QHash<qint64,int> momentsIndex,bool hasMore);
    void momentUpdated(const LocalMoment &moment);
    void likeStatusChanged(qint64 momentId, bool isLiked, bool success);
    void commentAdded(qint64 momentId, bool success, const QString& error = QString());
    void commentDeleted(qint64 momentId, qint64 commentId, bool success, const QString& error = QString());
    void mediaDownloadStatusUpdated(bool success, const QString& error = QString());
    void expiredMomentsCleared(int count, const QString& error = QString());
    void dbError(int reqId, const QString& error);

private slots:
    // 数据库操作结果处理
    void onMomentSaved(int reqId, bool ok, const QString& reason);
    void onMomentsLoaded(int reqId, const QVector<LocalMoment>& moments);
    void onMomentInteractLoaded(int reqId, const LocalMomentInteract& interact);
    void onExpiredMomentsCleared(int reqId, int count, const QString& reason);
    void onDbError(int reqId, const QString& error);

private:
    // 辅助方法
    int generateReqId();
    void connectSignals();
    LocalMoment createMoment(const QString& content, const QVector<MomentImageInfo>& images, const MomentImageInfo& video, int privacyType);
    qint64 generateMomentId(); // 生成唯一朋友圈ID

private:
    DatabaseManager* m_dbManager;            // 数据库管理器
    LocalMomentTable* m_localMomentTable;    // 朋友圈表操作接口
    QAtomicInteger<int> m_reqIdCounter;      // 线程安全请求ID计数器
    QHash<int, QString> m_pendingOperations; // 待处理操作映射

    User m_currentUser;                    // 当前登录用户
    QVector<LocalMoment> m_moments;        // 当前加载的朋友圈列表
    QHash<qint64,int> m_momentsIndexs;     // 朋友圈缓存的索引用于快速拿取某个朋友圈
    bool m_loading;                        // 加载状态标记
    int m_currentOffset;                   // 分页加载偏移量
    bool m_hasMore;                        // 是否有更多数据
    int m_limit = 0;                       // 本次加载朋友圈数量，用于比较返回数量从而确定是否还有更多朋友圈
    QHash<qint64, MomentCommentInfo> m_pendingAddComments; // 待添加的评论缓存：momentId -> 评论数据

};

#endif // LOCALMOMENTCONTROLLER_H
