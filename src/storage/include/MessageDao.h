#ifndef MESSAGEDAO_H
#define MESSAGEDAO_H

#include <QList>
#include "Message.h"
#include "MediaItem.h"

class MessageDao
{
public:
    MessageDao();

    // 获取会话消息（带发送者信息），按 msg_time DESC 排序，支持分页
    QList<Message> fetchMessages(qint64 conversationId, int limit, int offset = 0);

    // 获取会话中的媒体项（图片、视频）
    QList<MediaItem> fetchMediaItems(qint64 conversationId);
};

#endif // MESSAGEDAO_H