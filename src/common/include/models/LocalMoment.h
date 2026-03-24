#pragma once
#include "ORM_Macros.h"
#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QJsonDocument>

class LocalMomentDb {
    Q_GADGET
    ORM_MODEL(LocalMomentDb, "local_moment")

    ORM_FIELD(qint64, moment_id)           // INTEGER PRIMARY KEY
    ORM_FIELD(qint64, user_id)             // INTEGER NOT NULL
    ORM_FIELD(QString, username)           // TEXT NOT NULL
    ORM_FIELD(QString, avatar_local_path)  // TEXT
    ORM_FIELD(QString, avatar_url)         // TEXT
    ORM_FIELD(QString, content)            // TEXT DEFAULT ''
    ORM_FIELD(QString, video_local_path)   // TEXT DEFAULT ''
    ORM_FIELD(QString, video_url)          // TEXT DEFAULT ''
    ORM_FIELD(int, video_download_status)  // INTEGER DEFAULT 0
    ORM_FIELD(QString, images_info)        // TEXT DEFAULT '[]'
    ORM_FIELD(int, privacy_type)           // INTEGER DEFAULT 0
    ORM_FIELD(int, is_deleted)             // INTEGER DEFAULT 0
    ORM_FIELD(int, sync_status)            // INTEGER DEFAULT 0
    ORM_FIELD(qint64, expire_time)         // INTEGER
    ORM_FIELD(qint64, create_time)         // INTEGER
    ORM_FIELD(qint64, local_update_time)   // INTEGER DEFAULT (strftime('%s', 'now'))

public:
    LocalMomentDb() = default;
};

class LocalMomentInteractDb {
    Q_GADGET
    ORM_MODEL(LocalMomentInteractDb, "local_moment_interact")

    ORM_FIELD(qint64, interact_id)          // INTEGER PRIMARY KEY AUTOINCREMENT
    ORM_FIELD(qint64, moment_id)            // INTEGER NOT NULL UNIQUE
    ORM_FIELD(QString, likes)               // TEXT DEFAULT '[]'
    ORM_FIELD(QString, comments)            // TEXT DEFAULT '[]'
    ORM_FIELD(qint64, local_update_time)    // INTEGER DEFAULT (strftime('%s', 'now'))

public:
    LocalMomentInteractDb() = default;
};


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
    qint64 commentId = -1;     // 评论ID
    qint64 userId = -1;        // 评论用户ID
    QString username;         // 评论用户昵称
    QString content;          // 文字内容
    MomentImageInfo image;    // 评论图片
    qint64 replyUserId = -1;   // 回复目标用户ID
    QString replyUsername;    // 回复目标用户名
    qint64 createTime = -1;    // 评论时间戳
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
    qint64 interactId = -1;
    qint64 momentId = -1;
    QVector<MomentLikeInfo> likes;
    QVector<MomentCommentInfo> comments;
    qint64 localUpdateTime = -1;

    bool isValid() const { return !(momentId<0); }

    // 将 likes 列表转换为 JSON 数组字符串
    QString likesToJson() const {
        QJsonArray array;
        for (const auto& like : likes) {
            array.append(like.toJson());
        }
        return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    }

    // 将 comments 列表转换为 JSON 数组字符串
    QString commentsToJson() const {
        QJsonArray array;
        for (const auto& comment : comments) {
            array.append(comment.toJson());
        }
        return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    }

    // 从两个独立的 JSON 字符串恢复互动数据（分别对应 likes 和 comments 字段）
    static LocalMomentInteract fromDb(const LocalMomentInteractDb& db)
    {
        LocalMomentInteract interact;
        interact.interactId = db.interact_idValue();
        interact.momentId = db.moment_idValue();

        // 解析点赞列表
        QJsonDocument doc = QJsonDocument::fromJson(db.likesValue().toUtf8());
        if (doc.isArray()) {
            for (const auto& val : doc.array()) {
                interact.likes.append(MomentLikeInfo::fromJson(val.toObject()));
            }
        }

        // 解析评论列表
        doc = QJsonDocument::fromJson(db.commentsValue().toUtf8());
        if (doc.isArray()) {
            for (const auto& val : doc.array()) {
                interact.comments.append(MomentCommentInfo::fromJson(val.toObject()));
            }
        }

        return interact;
    }
};

// 朋友圈主模型
struct LocalMoment {
    qint64 momentId = -1;           // 服务端唯一ID
    qint64 userId = -1;             // 发布者ID
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

    bool isValid() const { return !(momentId < 0); }
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
        for (const auto& imgObj : std::as_const(array)) {
            images.append(MomentImageInfo::fromJson(imgObj.toObject()));
        }
    }
};

