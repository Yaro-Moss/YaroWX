#pragma once
#include "FormatFileSize.h"
#include "ORM_Macros.h"
#include <QObject>

// 消息类型枚举
enum class MessageType {
    TEXT = 0,
    IMAGE = 1,
    VIDEO = 2,
    FILE = 3,
    VOICE = 4
};

// 下载状态枚举
enum class DownloadStatus {
    NOT_DOWNLOADED = 0,
    DOWNLOADING   = 1,
    COMPLETED     = 2,
    FAILED        = 3
};

class Message {
    Q_GADGET
    ORM_MODEL(Message, "messages")

    ORM_FIELD(qint64, message_id)          // INTEGER PRIMARY KEY
    ORM_FIELD(qint64, conversation_id)     // INTEGER NOT NULL
    ORM_FIELD(qint64, sender_id)           // INTEGER NOT NULL
    ORM_FIELD(qint64, consignee_id)        // INTEGER NOT NULL
    ORM_FIELD(int, type)                   // INTEGER NOT NULL
    ORM_FIELD(QString, content)            // TEXT
    ORM_FIELD(QString, file_path)          // TEXT
    ORM_FIELD(QString, file_url)           // TEXT
    ORM_FIELD(qint64, file_size)           // INTEGER
    ORM_FIELD(int, duration)               // INTEGER
    ORM_FIELD(QString, thumbnail_path)     // TEXT
    ORM_FIELD(qint64, msg_time)            // INTEGER

    ORM_FIELD(int, thumb_download_status)  // 缩略图下载状态，默认0,0未下载 1下载中 2已完成 3失败
    ORM_FIELD(int, file_download_status)   // 文件下载状态，默认0

private:
    QVariant m_senderName;
    QVariant m_avatar;

public:
    void setSenderName(QString senderName) { m_senderName = senderName; }
    void setAvatar(QString avatar)         { m_avatar = avatar; }
    QString senderName() const { return m_senderName.toString(); }
    QString avatar() const     { return m_avatar.toString(); }

    Message() = default;

    bool isValid() const {
        return message_idValue() > 0 && conversation_idValue() > 0 && sender_idValue() > 0;
    }

    // 便捷方法：消息类型判断
    bool isText() const  { return static_cast<MessageType>(typeValue()) == MessageType::TEXT; }
    bool isImage() const { return static_cast<MessageType>(typeValue()) == MessageType::IMAGE; }
    bool isVideo() const { return static_cast<MessageType>(typeValue()) == MessageType::VIDEO; }
    bool isFile() const  { return static_cast<MessageType>(typeValue()) == MessageType::FILE; }
    bool isVoice() const { return static_cast<MessageType>(typeValue()) == MessageType::VOICE; }
    bool isMedia() const { return isImage() || isVideo() || isVoice(); }
    bool hasFile() const { return !file_pathValue().isEmpty() || !file_urlValue().isEmpty(); }
    bool hasThumbnail() const { return !thumbnail_pathValue().isEmpty(); }

    // 下载状态便捷方法
    DownloadStatus thumbDownloadStatus() const {
        return static_cast<DownloadStatus>(thumb_download_statusValue());
    }
    DownloadStatus fileDownloadStatus() const {
        return static_cast<DownloadStatus>(file_download_statusValue());
    }
    bool isThumbDownloaded() const { return thumbDownloadStatus() == DownloadStatus::COMPLETED; }
    bool isFileDownloaded() const  { return fileDownloadStatus() == DownloadStatus::COMPLETED; }
    bool isThumbDownloadFailed() const { return thumbDownloadStatus() == DownloadStatus::FAILED; }
    bool isFileDownloadFailed() const  { return fileDownloadStatus() == DownloadStatus::FAILED; }

    // 原有便捷方法
    QString formattedFileSize() const {
        return formatFileSize(file_sizeValue());
    }

    QString formattedDuration() const {
        if (durationValue() <= 0) return "0:00:00";
        int hours   = durationValue() / 3600;
        int minutes = (durationValue() % 3600) / 60;
        int seconds = durationValue() % 60;
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    bool isOwn(qint64 currentUserId) const {
        return sender_id() == currentUserId;
    }
};