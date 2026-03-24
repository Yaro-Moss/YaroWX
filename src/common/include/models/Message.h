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


private:
    QVariant m_senderName ;
    QVariant m_avatar;


public:
    void setSenderName(QString senderName){
        m_senderName = senderName;
    }
    void setAvatar(QString avatar){
        m_avatar = avatar;
    }
    QString senderName() const {
        return m_senderName.toString();
    }
    QString avatar()const{
        return m_avatar.toString();
    }

    Message() = default;

        bool isValid() const {
            return message_idValue() > 0 && conversation_idValue() > 0 && sender_idValue() > 0;
    }

    // 便捷方法
    bool isText() const { return static_cast<MessageType>(typeValue()) == MessageType::TEXT; }
    bool isImage() const { return static_cast<MessageType>(typeValue()) == MessageType::IMAGE; }
    bool isVideo() const { return static_cast<MessageType>(typeValue()) == MessageType::VIDEO; }
    bool isFile() const { return static_cast<MessageType>(typeValue()) == MessageType::FILE; }
    bool isVoice() const { return static_cast<MessageType>(typeValue()) == MessageType::VOICE; }
    bool isMedia() const { return isImage() || isVideo() || isVoice(); }
    bool hasFile() const { return !file_pathValue().isEmpty() || !file_urlValue().isEmpty(); }
    bool hasThumbnail() const { return !thumbnail_pathValue().isEmpty(); }

    QString formattedFileSize() const {
        return formatFileSize(file_sizeValue());
    }

    QString formattedDuration() const {
        if (durationValue() <= 0) return "0:00:00";

        // 1. 拆分时间单位：总秒数 → 时、分、秒
        int hours = durationValue()/ 3600;
        int remainingSeconds = durationValue() % 3600;
        int minutes = remainingSeconds / 60;
        int seconds = remainingSeconds % 60;

        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    bool isOwn(qint64 currentUserId) const {
        return sender_id() == currentUserId;
    }
};
