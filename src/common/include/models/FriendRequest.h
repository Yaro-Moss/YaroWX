#pragma once
#include "ORM_Macros.h"
#include <QObject>

class FriendRequest {
    Q_GADGET
    ORM_MODEL(FriendRequest, "friend_requests")

    ORM_FIELD(qint64, id)               // 申请ID（主键）
    ORM_FIELD(qint64, from_user_id)     // 申请人ID
    ORM_FIELD(QString, message)         // 附言
    ORM_FIELD(int, status)              // 状态：0-待处理，1-已同意，2-已拒绝
    ORM_FIELD(qint64, created_at)       // 申请发起时间戳（秒）

    ORM_FIELD(QString, from_nickname)   // 申请人昵称
    ORM_FIELD(QString, from_avatar)     // 申请人头像URL
    ORM_FIELD(QString, from_account)    // 申请人账号

    ORM_FIELD(int, is_read)             // 是否已读：0-未读，1-已读
    ORM_FIELD(int, local_processed)     // 本地处理标记：0-未处理，1-已处理（防重复）

public:
    FriendRequest() = default;

    // 判断记录是否有效（主键存在）
    bool isValid() const {
        return m_id.isValid();
    }

    // 判断是否为待处理状态
    bool isPending() const {
        return !m_status.isNull() && m_status.value<int>() == 0;
    }

    // 标记为已读
    void markAsRead() {
        setField("is_read", 1);
    }

    // 标记为已本地处理
    void markAsProcessed() {
        setField("local_processed", 1);
    }
};