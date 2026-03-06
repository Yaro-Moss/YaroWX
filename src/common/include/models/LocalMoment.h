#ifndef LOCALMOMENT_H
#define LOCALMOMENT_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QJsonDocument>

// 图片信息结构体（对应数据库中images_info的JSON项）
struct MomentImageInfo {
    QString url;              // 网络URL
    QString localPath;        // 本地路径
    int downloadStatus = 0;   // 0-未下载 1-下载中 2-已下载 3-下载失败
    int sort = 0;             // 排序

    // 序列化/反序列化
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["url"] = url;
        obj["localPath"] = localPath;
        obj["downloadStatus"] = downloadStatus;
        obj["sort"] = sort;
        return obj;
    }

    static MomentImageInfo fromJson(const QJsonObject& obj) {
        MomentImageInfo info;
        info.url = obj["url"].toString();
        info.localPath = obj["localPath"].toString();
        info.downloadStatus = obj["downloadStatus"].toInt();
        info.sort = obj["sort"].toInt();
        return info;
    }
};

// 点赞信息结构体
struct MomentLikeInfo {
    qint64 userId = 0;        // 点赞用户ID
    QString username;         // 点赞用户昵称
    QString avatarLocalPath;  // 点赞用户头像本地路径

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["userId"] = userId;
        obj["username"] = username;
        obj["avatarLocalPath"] = avatarLocalPath;
        return obj;
    }

    static MomentLikeInfo fromJson(const QJsonObject& obj) {
        MomentLikeInfo info;
        info.userId = obj["userId"].toInteger(0);
        info.username = obj["username"].toString();
        info.avatarLocalPath = obj["avatarLocalPath"].toString();
        return info;
    }
};

// 评论信息结构体
struct MomentCommentInfo {
    qint64 commentId = 0;     // 评论ID
    qint64 userId = 0;        // 评论用户ID
    QString username;         // 评论用户昵称
    QString content;          // 文字内容
    MomentImageInfo image;    // 评论图片
    qint64 replyUserId = 0;   // 回复目标用户ID
    QString replyUsername;    // 回复目标用户名
    qint64 createTime = 0;    // 评论时间戳
    bool isDeleted = false;   // 是否删除
    QString avatarLocalPath;  // 点赞用户头像本地路径


    QJsonObject toJson() const {
        QJsonObject obj;
        obj["commentId"] = commentId;
        obj["userId"] = userId;
        obj["username"] = username;
        obj["content"] = content;
        obj["avatarLocalPath"] = avatarLocalPath;
        
        QJsonObject img;
        img = image.toJson();
        obj["image"] = img;
        
        obj["replyUserId"] = replyUserId;
        obj["replyUsername"] = replyUsername;
        obj["createTime"] = createTime;
        obj["isDeleted"] = isDeleted;
        return obj;
    }

    static MomentCommentInfo fromJson(const QJsonObject& obj) {
        MomentCommentInfo info;
        info.commentId = obj["commentId"].toInteger(0);
        info.userId = obj["userId"].toInteger(0);
        info.username = obj["username"].toString();
        info.content = obj["content"].toString();
        info.avatarLocalPath = obj["avatarLocalPath"].toString();
        
        QJsonObject img = obj["image"].toObject();
        info.image = MomentImageInfo::fromJson(img);

        info.replyUserId = obj["replyUserId"].toInt();
        info.replyUsername = obj["replyUsername"].toString();
        info.createTime = obj["createTime"].toInt();
        info.isDeleted = obj["isDeleted"].toBool();
        return info;
    }
};

// 朋友圈互动信息（点赞+评论）
struct LocalMomentInteract {
    qint64 momentId = 0;                  // 朋友圈ID
    QVector<MomentLikeInfo> likes;        // 点赞列表
    QVector<MomentCommentInfo> comments;  // 评论列表
    qint64 localUpdateTime = 0;           // 本地更新时间

    bool isValid() const { return momentId != 0; }

    QJsonObject toJson() const {
        QJsonObject obj;
        QJsonArray likeArray, commentArray;
        
        for (const auto& like : likes) likeArray.append(like.toJson());
        for (const auto& comment : comments) commentArray.append(comment.toJson());
        
        obj["likes"] = likeArray;
        obj["comments"] = commentArray;
        return obj;
    }

    static LocalMomentInteract fromJson(qint64 momentId, const QString& jsonStr) {
        LocalMomentInteract interact;
        interact.momentId = momentId;
        
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isObject()) return interact;
        
        QJsonObject obj = doc.object();
        QJsonArray likeArray = obj["likes"].toArray();
        QJsonArray commentArray = obj["comments"].toArray();
        
        for (const auto& likeObj : likeArray) {
            interact.likes.append(MomentLikeInfo::fromJson(likeObj.toObject()));
        }
        for (const auto& commentObj : commentArray) {
            interact.comments.append(MomentCommentInfo::fromJson(commentObj.toObject()));
        }
        return interact;
    }
};

// 朋友圈主模型
struct LocalMoment {
    qint64 momentId = 0;           // 服务端唯一ID
    qint64 userId = 0;             // 发布者ID
    QString username;              // 发布者昵称
    QString avatarLocalPath;       // 发布者头像本地路径
    QString avatarUrl;             // 发布者头像网络URL
    QString content;               // 文本内容

    // 视频相关
    QString videoLocalPath;        // 视频本地路径
    QString videoUrl;              // 视频网络URL
    int videoDownloadStatus = 0;   // 视频下载状态

    // 图片相关
    QVector<MomentImageInfo> images; // 图片列表

    // 权限与状态
    int privacyType = 0;           // 0-公开 1-仅好友 2-仅部分 3-不给谁看
    bool isDeleted = false;        // 是否删除

    // 缓存控制
    int syncStatus = 0;            // 0-已同步 1-待同步 2-同步失败
    qint64 expireTime = 0;         // 过期时间戳
    qint64 createTime = 0;         // 发布时间戳
    qint64 localUpdateTime = 0;    // 本地更新时间

    LocalMomentInteract interact;   // 该动态的互动信息

    bool isValid() const { return momentId != 0; }
    bool hasVideo() const { return !videoUrl.isEmpty(); }
    bool hasImages() const { return !images.isEmpty(); }
    bool isPureText() const { return !content.isEmpty() && !hasVideo() && !hasImages(); }

    // 序列化图片列表为JSON字符串（存入数据库）
    QString imagesToJson() const {
        QJsonArray array;
        for (const auto& img : images) array.append(img.toJson());
        return QJsonDocument(array).toJson(QJsonDocument::Compact);
    }

    // 从JSON字符串解析图片列表
    void imagesFromJson(const QString& jsonStr) {
        images.clear();
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isArray()) return;
        
        QJsonArray array = doc.array();
        for (const auto& imgObj : array) {
            images.append(MomentImageInfo::fromJson(imgObj.toObject()));
        }
    }
};

#endif // LOCALMOMENT_H
