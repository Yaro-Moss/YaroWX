#ifndef CONVERSATIONCONTROLLER_H
#define CONVERSATIONCONTROLLER_H

#include <QObject>
#include <QAtomicInteger>
#include <QHash>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include "Contact.h"
#include "ChatListModel.h"

class ConversationController : public QObject
{
    Q_OBJECT

public:
    explicit ConversationController(QObject* parent = nullptr);
    ~ConversationController();

    ChatListModel* chatListModel() const { return m_chatListModel; }

    // 异步操作
    void loadConversations();
    void createSingleChat(const Contact &contact);
    void createSingleChat(qint64 userId);  // 新增：只传 userId 的版本
    void createGroupChat(qint64 groupId);

    // 菜单操作
    void clearUnreadCount(qint64 conversationId);
    void deleteConversation(qint64 conversationId);
    void handleToggleTop(qint64 conversationId);
    void handleToggleMute(qint64 conversationId);
    void handleOpenInWindow(qint64 conversationId);
    void handleDelete(qint64 conversationId);
    void handletoggleReadStatus(qint64 conversationId);

    void setCurrentConversationId(qint64 conversationId);
    Conversation getConversationFromModel(qint64 id);
    qint64 getConversationIdByTarget(int chatType, qint64 targetId);
signals:
    void messageSave();
    void createChatSuccess(qint64 id);

private:
    ChatListModel* m_chatListModel;
    qint64 m_currentConversationId = -1;

};

#endif // CONVERSATIONCONTROLLER_H
