#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QThread>

class UserTable;
class ContactTable;
class GroupTable;
class GroupMemberTable;
class ConversationTable;
class MessageTable;
class MediaCacheTable;
class LocalMomentTable;

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    UserTable *userTable() const { return m_userTable; }
    ContactTable *contactTable() const { return m_contactTable; }
    GroupTable *groupTable() const { return m_groupTable; }
    GroupMemberTable *groupMemberTable() const { return m_groupMemberTable; }
    ConversationTable *conversationTable() const { return m_conversationTable; }
    MessageTable *messageTable() const { return m_messageTable; }
    MediaCacheTable *mediaCacheTable() const { return m_mediaCacheTable; }
    LocalMomentTable *localMomentTable() const { return m_localMomentTable; }

    void start();
    void stop();

private:
    QThread* m_dbThread = nullptr;

    UserTable *m_userTable = nullptr;
    ContactTable *m_contactTable = nullptr;
    GroupTable *m_groupTable = nullptr;
    GroupMemberTable *m_groupMemberTable = nullptr;
    ConversationTable *m_conversationTable = nullptr;
    MessageTable *m_messageTable = nullptr;
    MediaCacheTable *m_mediaCacheTable = nullptr;
    LocalMomentTable *m_localMomentTable = nullptr;

};

#endif // DATABASEMANAGER_H
