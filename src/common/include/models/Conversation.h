#pragma once
#include "ORM_Macros.h"
#include <QObject>

class Conversation {
    Q_GADGET
    ORM_MODEL(Conversation, "conversations")

    ORM_FIELD(qint64, conversation_id)       // INTEGER PRIMARY KEY AUTOINCREMENT
    ORM_FIELD(qint64, group_id)              // INTEGER UNIQUE
    ORM_FIELD(qint64, user_id)               // INTEGER UNIQUE
    ORM_FIELD(int, type)                     // INTEGER NOT NULL
    ORM_FIELD(QString, title)                // TEXT
    ORM_FIELD(QString, avatar)               // TEXT
    ORM_FIELD(QString, avatar_local_path)    // TEXT
    ORM_FIELD(QString, last_message_content) // TEXT
    ORM_FIELD(qint64, last_message_time)     // INTEGER
    ORM_FIELD(int, unread_count)             // INTEGER DEFAULT 0
    ORM_FIELD(int, is_top)                   // INTEGER DEFAULT 0

public:
    Conversation() = default;
    QVariant isGroup() const{
        return type() == 1;
    }

    QVariant targetId() const{
        return isGroup().toBool()? group_id():user_id();
    }
    bool isValid() const {
        return !m_conversation_id.isNull();
    }

};



// 自定义数据角色
enum ConversationRole {
    ConversationIdRole = Qt::UserRole + 1,
    GroupIdRole,
    UserIdRole,
    TypeRole,
    TitleRole,
    AvatarRole,
    AvatarLocalPathRole,
    LastMessageContentRole,
    LastMessageTimeRole,
    UnreadCountRole,
    IsTopRole,
    IsGroupRole,
    TargetIdRole
};
