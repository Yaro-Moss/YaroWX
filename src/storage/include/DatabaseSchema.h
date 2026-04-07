#ifndef DATABASESCHEMA_H
#define DATABASESCHEMA_H

#include <QString>

class DatabaseSchema {
public:
    // 表名常量（新增朋友圈相关表名）
    static const char* TABLE_CURRENT_USER;
    static const char* TABLE_CONTACTS;
    static const char* TABLE_GROUP_MEMBERS;
    static const char* TABLE_GROUPS;
    static const char* TABLE_CONVERSATIONS;
    static const char* TABLE_MESSAGES;
    static const char* TABLE_MEDIA_CACHE;
    static const char* TABLE_LOCAL_MOMENT;
    static const char* TABLE_LOCAL_MOMENT_INTERACT;
    static const char* TABLE_FRIEND_REQUESTS;

    // 创建表的SQL语句
    static QString getCreateTableUser();
    static QString getCreateTableContacts();
    static QString getCreateTableGroupMembers();
    static QString getCreateTableGroups();
    static QString getCreateTableConversations();
    static QString getCreateTableMessages();
    static QString getCreateTableMediaCache();
    static QString getCreateTableLocalMoment();          
    static QString getCreateTableLocalMomentInteract();   
    static QString getCreateTableFriendRequests();

    static QStringList getCreateTriggers();
    static QString getCreateIndexes();
};

#endif // DATABASESCHEMA_H