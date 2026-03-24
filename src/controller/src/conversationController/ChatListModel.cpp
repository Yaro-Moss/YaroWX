#include "ChatListModel.h"
#include <QDebug>

ChatListModel::ChatListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChatListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_conversationIds.size();
}

QVariant ChatListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_conversationIds.size())
        return QVariant();

    qint64 id = m_conversationIds.at(index.row());
    const Conversation &conversation = m_conversations.value(id);

    switch (role) {
    case ConversationIdRole:
        return conversation.conversation_id();
    case GroupIdRole:
        return conversation.group_id();
    case UserIdRole:
        return conversation.user_id();
    case TypeRole:
        return conversation.type();
    case TitleRole:
        return conversation.title();
    case AvatarRole:
        return conversation.avatar();
    case AvatarLocalPathRole:
        return conversation.avatar_local_path();
    case LastMessageContentRole:
        return conversation.last_message_content();
    case LastMessageTimeRole:
        return conversation.last_message_time();
    case UnreadCountRole:
        return conversation.unread_count();
    case IsTopRole:
        return conversation.is_top();
    case IsGroupRole:
        return conversation.isGroup();
    case TargetIdRole:
        return conversation.targetId();
    default:
        return QVariant();
    }
}

bool ChatListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_conversationIds.size())
        return false;

    qint64 id = m_conversationIds.at(index.row());
    Conversation &conversation = m_conversations[id]; // 通过ID获取可修改引用
    bool changed = false;

    switch (role) {
    case TitleRole:
        if (conversation.title() != value.toString()) {
            conversation.settitle(value.toString());
            changed = true;
        }
        break;
    case AvatarRole:
        if (conversation.avatar() != value.toString()) {
            conversation.setavatar(value.toString());
            changed = true;
        }
        break;
    case AvatarLocalPathRole:
        if (conversation.avatar_local_path() != value.toString()) {
            conversation.setavatar_local_path(value.toString());
            changed = true;
        }
        break;
    case LastMessageContentRole:
        if (conversation.last_message_content() != value.toString()) {
            conversation.setlast_message_content(value.toString());
            changed = true;
        }
        break;
    case LastMessageTimeRole:
        if (conversation.last_message_time() != value.toLongLong()) {
            conversation.setlast_message_time(value.toLongLong());
            changed = true;
        }
        break;
    case UnreadCountRole:
        if (conversation.unread_count() != value.toInt()) {
            conversation.setunread_count(value.toInt());
            changed = true;
        }
        break;
    case IsTopRole:
        if (conversation.is_top() != value.toInt()) {
            conversation.setis_top(value.toInt());
            changed = true;
        }
        break;
    default:
        return false;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}

QHash<int, QByteArray> ChatListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ConversationIdRole] = "conversationId";
    roles[GroupIdRole] = "groupId";
    roles[UserIdRole] = "userId";
    roles[TypeRole] = "type";
    roles[TitleRole] = "title";
    roles[AvatarRole] = "avatar";
    roles[AvatarLocalPathRole] = "avatarLocalPath";
    roles[LastMessageContentRole] = "lastMessageContent";
    roles[LastMessageTimeRole] = "lastMessageTime";
    roles[UnreadCountRole] = "unreadCount";
    roles[IsTopRole] = "isTop";
    roles[IsGroupRole] = "isGroup";
    roles[TargetIdRole] = "targetId";
    return roles;
}



int ChatListModel::findInsertPosition(qint64 id) const
{
    const Conversation &self = m_conversations.value(id);
    int count = 0;
    for (int i = 0; i < m_conversationIds.size(); ++i) {
        qint64 otherId = m_conversationIds.at(i);
        if (otherId == id) continue;   // 跳过自身
        const Conversation &other = m_conversations.value(otherId);
        // 如果 other 应排在 self 前面，则 self 的位置需后移一位
        if (lessThan(other, self))
            ++count;
    }
    return count;   // 返回从 0 开始的插入位置
}

void ChatListModel::reorderConversation(qint64 id)
{
    int oldRow = m_conversationIds.indexOf(id);
    if (oldRow < 0) return;

    int newRow = findInsertPosition(id);
    if (newRow == oldRow) return;   // 位置没变，不需要移动

    // 通知视图即将移动行
    if (oldRow < newRow) {
        // 向下移动（到更大索引）
        beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow + 1);
    } else {
        // 向上移动（到更小索引）
        beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow);
    }
    // 移动列表中的 ID
    m_conversationIds.move(oldRow, newRow);
    endMoveRows();
}

void ChatListModel::loadConversations(const QList<Conversation>& conversations)
{
    beginResetModel();
    m_conversations.clear();
    m_conversationIds.clear();

    for (const auto& conv : conversations) {
        m_conversations.insert(conv.conversation_idValue(), conv);
        // 直接追加，因为数据库已经排好序
        m_conversationIds.append(conv.conversation_idValue());
    }
    endResetModel();
}

void ChatListModel::addConversation(const Conversation &conversation)
{
    qint64 id = conversation.conversation_idValue();
    if (m_conversations.contains(id)) {
        updateConversation(conversation);
        return;
    }

    // 先插入到哈希表
    m_conversations.insert(id, conversation);

    // 计算应该插入的位置
    int newRow = findInsertPosition(id);
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_conversationIds.insert(newRow, id);
    endInsertRows();
}

void ChatListModel::updateConversation(const Conversation &conversation)
{
    qint64 id = conversation.conversation_idValue();
    if (!m_conversations.contains(id))
        return;

    // 更新数据
    m_conversations[id] = conversation;

    // 找到对应行并通知更新
    int row = m_conversationIds.indexOf(id);
    if (row != -1) {
        QModelIndex modelIndex = createIndex(row, 0);
        emit dataChanged(modelIndex, modelIndex);
    }
}

void ChatListModel::updateLastMessage(qint64 conversationId, const QString &message, qint64 time)
{
    if (!m_conversations.contains(conversationId)) return;

    Conversation &conversation = m_conversations[conversationId];
    conversation.setlast_message_content(message);
    conversation.setlast_message_time(time);

    int row = m_conversationIds.indexOf(conversationId);
    if (row != -1)
        emit dataChanged(createIndex(row, 0), createIndex(row, 0),
                         {LastMessageContentRole, LastMessageTimeRole});

    // 重新排序（时间变化可能影响所有会话的顺序）
    reorderConversation(conversationId);
}

void ChatListModel::updateUnreadCount(qint64 conversationId, int count)
{
    if (!m_conversations.contains(conversationId))
        return;

    Conversation &conversation = m_conversations[conversationId];
    conversation.setunread_count(count);

    int row = m_conversationIds.indexOf(conversationId);
    if (row != -1) {
        QModelIndex modelIndex = createIndex(row, 0);
        emit dataChanged(modelIndex, modelIndex, {UnreadCountRole});
    }
}

void ChatListModel::updateTopStatus(qint64 conversationId, bool isTop)
{
    if (!m_conversations.contains(conversationId)) return;

    Conversation &conversation = m_conversations[conversationId];
    conversation.setis_top(isTop);

    // 数据已改变，先通知该行的数据变化（可选，但移动后视图会刷新，此步可省略）
    int row = m_conversationIds.indexOf(conversationId);
    if (row != -1)
        emit dataChanged(createIndex(row, 0), createIndex(row, 0), {IsTopRole});

    // 重新排序
    reorderConversation(conversationId);
}

Conversation ChatListModel::getConversation(qint64 conversationId) const
{
    return m_conversations.value(conversationId); // 不存在则返回默认构造
}

Conversation ChatListModel::getConversationAt(int index) const
{
    if (index >= 0 && index < m_conversationIds.size()) {
        qint64 id = m_conversationIds.at(index);
        return m_conversations.value(id);
    }
    return Conversation();
}

void ChatListModel::removeConversation(qint64 conversationId)
{
    int index = m_conversationIds.indexOf(conversationId);
    if (index != -1) {
        beginRemoveRows(QModelIndex(), index, index);
        m_conversations.remove(conversationId);
        m_conversationIds.removeAt(index);
        endRemoveRows();
    }
}

void ChatListModel::clearAll()
{
    if (!m_conversationIds.isEmpty()) {
        beginResetModel();
        m_conversations.clear();
        m_conversationIds.clear();
        endResetModel();
    }
}

int ChatListModel::findConversationIndex(qint64 conversationId) const
{
    return m_conversationIds.indexOf(conversationId);
}

QModelIndex ChatListModel::getConversationIndex(qint64 conversationId) const
{
    int row = m_conversationIds.indexOf(conversationId);
    if (row != -1)
        return createIndex(row, 0);
    return QModelIndex();
}

QModelIndex ChatListModel::getConversationIndexByContactId(qint64 contactId) const
{
    // 遍历所有会话，查找匹配的 userId 或 groupId
    for (int row = 0; row < m_conversationIds.size(); ++row) {
        qint64 id = m_conversationIds.at(row);
        const Conversation &conv = m_conversations.value(id);
        if (conv.user_id() == contactId || conv.group_id() == contactId) {
            return createIndex(row, 0);
        }
    }
    return QModelIndex();
}
