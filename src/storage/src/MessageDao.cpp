#include "MessageDao.h"
#include "DbConnectionManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

MessageDao::MessageDao()
{
}

QList<Message> MessageDao::fetchMessages(qint64 conversationId, int limit, int offset)
{
    QList<Message> messages;
    auto dbPtr = DbConnectionManager::connectionForCurrentThread();
    if (!dbPtr || !dbPtr->isOpen()) {
        qWarning() << "MessageDao::fetchMessages: no database connection";
        return messages;
    }
    QSqlDatabase db = *dbPtr;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            m.message_id,
            m.conversation_id,
            m.sender_id,
            m.consignee_id,
            m.type,
            m.content,
            m.file_path,
            m.file_url,
            m.file_size,
            m.duration,
            m.thumbnail_path,
            m.msg_time,
            CASE WHEN c.user_id IS NOT NULL THEN c.remark_name ELSE u.nickname END AS senderName,
            u.avatar_local_path AS avatar
        FROM messages m
        INNER JOIN users u ON m.sender_id = u.user_id
        LEFT JOIN contacts c ON m.sender_id = c.user_id
        WHERE m.conversation_id = ?
        ORDER BY m.msg_time DESC
        LIMIT ? OFFSET ?
    )");
    query.addBindValue(conversationId);
    query.addBindValue(limit);
    query.addBindValue(offset);

    if (!query.exec()) {
        qWarning() << "MessageDao::fetchMessages query failed:" << query.lastError().text();
        return messages;
    }

    while (query.next()) {
        Message msg;
        msg.setmessage_id(query.value("message_id").toLongLong());
        msg.setconversation_id(query.value("conversation_id").toLongLong());
        msg.setsender_id(query.value("sender_id").toLongLong());
        msg.setconsignee_id(query.value("consignee_id").toLongLong());
        msg.settype(query.value("type").toInt());
        msg.setcontent(query.value("content").toString());
        msg.setfile_path(query.value("file_path").toString());
        msg.setfile_url(query.value("file_url").toString());
        msg.setfile_size(query.value("file_size").toLongLong());
        msg.setduration(query.value("duration").toInt());
        msg.setthumbnail_path(query.value("thumbnail_path").toString());
        msg.setmsg_time(query.value("msg_time").toLongLong());

        msg.setSenderName(query.value("senderName").toString());
        msg.setAvatar(query.value("avatar").toString());

        messages.append(msg);
    }
    return messages;
}

QList<MediaItem> MessageDao::fetchMediaItems(qint64 conversationId)
{
    QList<MediaItem> items;
    auto dbPtr = DbConnectionManager::connectionForCurrentThread();
    if (!dbPtr || !dbPtr->isOpen()) {
        qWarning() << "MessageDao::fetchMediaItems: no database connection";
        return items;
    }
    QSqlDatabase db = *dbPtr;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            message_id,
            file_path,
            file_url,
            thumbnail_path,
            msg_time,
            type
        FROM messages
        WHERE conversation_id = ? AND (type = ? OR type = ?)
        ORDER BY msg_time DESC
    )");
    query.addBindValue(conversationId);
    query.addBindValue(static_cast<int>(MessageType::IMAGE));
    query.addBindValue(static_cast<int>(MessageType::VIDEO));

    if (!query.exec()) {
        qWarning() << "MessageDao::fetchMediaItems query failed:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        MediaItem item;
        item.messageId = query.value("message_id").toLongLong();
        item.sourceMediaPath = query.value("file_path").toString();
        if (item.sourceMediaPath.isEmpty())
            item.sourceMediaPath = query.value("file_url").toString();
        item.thumbnailPath = query.value("thumbnail_path").toString();
        item.timestamp = query.value("msg_time").toLongLong();
        int type = query.value("type").toInt();
        item.mediaType = (type == static_cast<int>(MessageType::IMAGE)) ? "image" : "video";
        items.append(item);
    }
    return items;
}