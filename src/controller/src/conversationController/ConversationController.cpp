#include "ConversationController.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include "Conversation.h"
#include "Group.h"                      // 新增：群组模型
#include "User.h"                        // 新增：用户模型（Contact 中可能需要）
#include "ORM.h"                         // 你的 ORM 框架头文件
#include "ConversationController.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

ConversationController::ConversationController(QObject* parent)
    : QObject(parent)
    , m_chatListModel(new ChatListModel(this))
{
}

ConversationController::~ConversationController()
{
}

// 异步加载所有会话
void ConversationController::loadConversations()
{
    // 使用 QFutureWatcher 监控后台线程的查询结果
    QFutureWatcher<QList<Conversation>> *watcher = new QFutureWatcher<QList<Conversation>>(this);
    connect(watcher, &QFutureWatcher<QList<Conversation>>::finished, this, [this, watcher]() {
        QList<Conversation> conversations = watcher->result();
        // 在主线程更新 UI 模型
        m_chatListModel->clearAll();
        m_chatListModel->loadConversations(conversations);
        emit messageSave();
        watcher->deleteLater();
    });

    // 后台线程执行数据库查询
    QFuture<QList<Conversation>> future = QtConcurrent::run([]() -> QList<Conversation> {
        Orm orm;
        // 按置顶降序、最后消息时间降序排序
        return orm.findWhere<Conversation>(
            QString(),                      // 无条件
            {},                             // 无参数
            "is_top DESC, last_message_time DESC"
            );
    });
    watcher->setFuture(future);
}

// 创建单聊会话（若已存在则直接返回）
void ConversationController::createSingleChat(const Contact &contact)
{
    qint64 userId = contact.user_idValue();
    QFutureWatcher<Conversation> *watcher = new QFutureWatcher<Conversation>(this);
    connect(watcher, &QFutureWatcher<Conversation>::finished, this, [this, watcher]() {
        Conversation conv = watcher->result();
        if (conv.isValid()) {
            // 如果模型中没有此会话，则添加
            if (m_chatListModel->findConversationIndex(conv.conversation_idValue()) == -1) {
                m_chatListModel->addConversation(conv);
            }
            emit createChatSuccess(conv.conversation_idValue());
        } else {
            qWarning() << "创建单聊会话失败，userId:" << conv.user_id();
        }
        watcher->deleteLater();
    });

    QFuture<Conversation> future = QtConcurrent::run([userId, contact]() -> Conversation {
        Orm orm;

        // 1. 检查是否已存在与这个用户的单聊会话（type=0）
        auto existing = orm.findWhere<Conversation>(
            "user_id = ? AND type = 0", {userId}
            );
        if (!existing.isEmpty()) {
            return existing.first();   // 已存在，直接返回
        }

        // 2. 不存在则创建新会话
        Conversation conv;
        conv.setuser_id(userId);
        conv.settype(0);
        // 标题优先使用备注名，否则用用户昵称
        QString title = contact.remark_nameValue();

        if (contact.user.isValid()) {
            conv.setavatar(contact.user.avatar());
            conv.setavatar_local_path(contact.user.avatar_local_path());
            if(title.isEmpty())
                title = contact.user.nicknameValue();
        } else {
            auto userOpt = orm.findById<User>(userId);
            if (userOpt) {
                conv.setavatar(userOpt->avatar());
                conv.setavatar_local_path(userOpt->avatar_local_path());
                if(title.isEmpty())
                    title = userOpt->nicknameValue();
            }
        }
        conv.settitle(title);
        conv.setunread_count(0);
        conv.setis_top(0);

        auto contactOpt = orm.findById<Contact>(userId);
        if (!contactOpt.has_value()) {
            Contact newContact;
            newContact.setuser_id(userId);
            if (orm.insert(newContact)) {
                qDebug() << "自动添加联系人成功";
            } else {
                QSqlError err = orm.lastError();
                if (err.isValid())
                    qDebug() << "插入失败：" << err.text();
                qWarning() << "自动添加联系人失败，无法创建会话";
                return Conversation();
            }
        }

        if (orm.insert(conv)) {
            return conv;
        }
        QSqlError err = orm.lastError();
        if (err.isValid()) {
            qDebug() << "插入失败：" << err.text();
        } else {
            qDebug() << "未找到记录";
        }
        return Conversation();  // 无效会话
    });
    watcher->setFuture(future);
}

// 创建群聊会话
void ConversationController::createGroupChat(qint64 groupId)
{
    QFutureWatcher<Conversation> *watcher = new QFutureWatcher<Conversation>(this);
    connect(watcher, &QFutureWatcher<Conversation>::finished, this, [this, watcher, groupId]() {
        Conversation conv = watcher->result();
        if (conv.conversation_idValue() > 0) {
            if (m_chatListModel->findConversationIndex(conv.conversation_idValue()) == -1) {
                m_chatListModel->addConversation(conv);
            }
            emit createChatSuccess(conv.conversation_idValue());
        } else {
            qWarning() << "创建群聊会话失败，groupId:" << groupId;
        }
        watcher->deleteLater();
    });

    QFuture<Conversation> future = QtConcurrent::run([groupId]() -> Conversation {
        Orm orm;

        // 检查是否已有该群聊会话
        auto existing = orm.findWhere<Conversation>(
            "group_id = ? AND type = 1", {groupId}
            );
        if (!existing.isEmpty()) {
            return existing.first();
        }

        // 获取群组信息
        auto groupOpt = orm.findById<Group>(groupId);
        if (!groupOpt) {
            return Conversation();
        }
        Group group = *groupOpt;

        Conversation conv;
        conv.setgroup_id(groupId);
        conv.settype(1);
        conv.settitle(group.group_name());
        conv.setavatar(group.avatar());
        conv.setavatar_local_path(group.avatar_local_path());
        conv.setunread_count(0);
        conv.setis_top(0);

        if (orm.insert(conv)) {
            return conv;
        }
        return Conversation();
    });
    watcher->setFuture(future);
}

// 清空未读消息数
void ConversationController::clearUnreadCount(qint64 conversationId)
{
    // 先不更新 UI，等待数据库操作成功后再更新
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, conversationId, watcher]() {
        bool success = watcher->result();
        if (success) {
            m_chatListModel->updateUnreadCount(conversationId, 0);
        } else {
            qWarning() << "Failed to clear unread count for conversation:" << conversationId;
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([conversationId]() -> bool {
        Orm orm;
        auto convOpt = orm.findById<Conversation>(conversationId);
        if (convOpt) {
            Conversation conv = *convOpt;
            conv.setunread_count(0);
            return orm.update(conv);
        }
        return false;
    });
    watcher->setFuture(future);
}

// 切换已读状态（你的原逻辑：未读>0则设为0，否则设为1）
void ConversationController::handletoggleReadStatus(qint64 conversationId)
{
    Conversation conv = m_chatListModel->getConversation(conversationId);
    if (conv.conversation_idValue() <= 0) return;

    int newUnread = (conv.unread_countValue() > 0) ? 0 : 1;

    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, conversationId, newUnread, watcher]() {
        if (watcher->result()) {
            m_chatListModel->updateUnreadCount(conversationId, newUnread);
        } else {
            qWarning() << "Failed to toggle read status for conversation:" << conversationId;
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([conversationId, newUnread]() -> bool {
        Orm orm;
        auto convOpt = orm.findById<Conversation>(conversationId);
        if (convOpt) {
            Conversation conv = *convOpt;
            conv.setunread_count(newUnread);
            return orm.update(conv);
        }
        return false;
    });
    watcher->setFuture(future);
}

// 切换置顶状态
void ConversationController::handleToggleTop(qint64 conversationId)
{
    Conversation conv = m_chatListModel->getConversation(conversationId);
    if (conv.conversation_idValue() <= 0) return;

    int newTop = conv.is_topValue() ? 0 : 1;

    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, conversationId, newTop, watcher]() {
        if (watcher->result()) {
            m_chatListModel->updateTopStatus(conversationId, newTop);
        } else {
            qWarning() << "Failed to toggle top status for conversation:" << conversationId;
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([conversationId, newTop]() -> bool {
        Orm orm;
        auto convOpt = orm.findById<Conversation>(conversationId);
        if (convOpt) {
            Conversation conv = *convOpt;
            conv.setis_top(newTop);
            return orm.update(conv);
        }
        return false;
    });
    watcher->setFuture(future);
}

// 删除会话
void ConversationController::deleteConversation(qint64 conversationId)
{
    // 先不删除 UI，等待数据库删除成功后再移除
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, conversationId, watcher]() {
        if (watcher->result()) {
            m_chatListModel->removeConversation(conversationId);
        } else {
            qWarning() << "Failed to delete conversation from database:" << conversationId;
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([conversationId]() -> bool {
        Orm orm;
        auto convOpt = orm.findById<Conversation>(conversationId);
        if (convOpt) {
            return orm.remove(*convOpt);
        }
        return false;
    });
    watcher->setFuture(future);
}

// 以下是其他菜单操作，暂时只打印调试信息
void ConversationController::handleToggleMute(qint64 conversationId)
{
    qDebug() << "Toggle mute for conversation:" << conversationId;
}

void ConversationController::handleOpenInWindow(qint64 conversationId)
{
    qDebug() << "Open conversation in window:" << conversationId;
}

void ConversationController::handleDelete(qint64 conversationId)
{
    deleteConversation(conversationId);
}

void ConversationController::setCurrentConversationId(qint64 conversationId)
{
    if (m_currentConversationId != conversationId) {
        m_currentConversationId = conversationId;
    }
}

Conversation ConversationController::getConversationFromModel(qint64 id)
{
    return m_chatListModel->getConversation(id);
}

qint64 ConversationController::getConversationIdByTarget(int chatType, qint64 targetId)
{
    Orm orm;
    QString condition;
    QVariantList params;

    if (chatType == 0) { // 单聊
        condition = "type = 0 AND user_id = ?";
        params << targetId;
    } else if (chatType == 1) { // 群聊
        condition = "type = 1 AND group_id = ?";
        params << targetId;
    } else {
        qWarning() << "getConversationIdByTarget: invalid chatType" << chatType;
        return -1;
    }

    auto conversations = orm.findWhere<Conversation>(condition, params);
    if (conversations.isEmpty()) {
        return -1;
    }
    return conversations.first().conversation_idValue();
}
