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
        return conversation.conversationId;
    case GroupIdRole:
        return conversation.groupId;
    case UserIdRole:
        return conversation.userId;
    case TypeRole:
        return conversation.type;
    case TitleRole:
        return conversation.title;
    case AvatarRole:
        return conversation.avatar;
    case AvatarLocalPathRole:
        return conversation.avatarLocalPath;
    case LastMessageContentRole:
        return conversation.lastMessageContent;
    case LastMessageTimeRole:
        return conversation.lastMessageTime;
    case UnreadCountRole:
        return conversation.unreadCount;
    case IsTopRole:
        return conversation.isTop;
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
        if (conversation.title != value.toString()) {
            conversation.title = value.toString();
            changed = true;
        }
        break;
    case AvatarRole:
        if (conversation.avatar != value.toString()) {
            conversation.avatar = value.toString();
            changed = true;
        }
        break;
    case AvatarLocalPathRole:
        if (conversation.avatarLocalPath != value.toString()) {
            conversation.avatarLocalPath = value.toString();
            changed = true;
        }
        break;
    case LastMessageContentRole:
        if (conversation.lastMessageContent != value.toString()) {
            conversation.lastMessageContent = value.toString();
            changed = true;
        }
        break;
    case LastMessageTimeRole:
        if (conversation.lastMessageTime != value.toLongLong()) {
            conversation.lastMessageTime = value.toLongLong();
            changed = true;
        }
        break;
    case UnreadCountRole:
        if (conversation.unreadCount != value.toInt()) {
            conversation.unreadCount = value.toInt();
            changed = true;
        }
        break;
    case IsTopRole:
        if (conversation.isTop != value.toBool()) {
            conversation.isTop = value.toBool();
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

void ChatListModel::addConversation(const Conversation &conversation)
{
    qint64 id = conversation.conversationId;
    if (m_conversations.contains(id)) {
        // 如果已存在，按更新处理（保持现有位置）
        updateConversation(conversation);
        return;
    }

    // 插入新会话：添加到哈希表，并追加到列表末尾（顺序由外部保证）
    beginInsertRows(QModelIndex(), m_conversationIds.size(), m_conversationIds.size());
    m_conversations.insert(id, conversation);
    m_conversationIds.append(id);
    endInsertRows();
}

void ChatListModel::updateConversation(const Conversation &conversation)
{
    qint64 id = conversation.conversationId;
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
    if (!m_conversations.contains(conversationId))
        return;

    Conversation &conversation = m_conversations[conversationId];
    conversation.lastMessageContent = message;
    conversation.lastMessageTime = time;

    int row = m_conversationIds.indexOf(conversationId);
    if (row != -1) {
        QModelIndex modelIndex = createIndex(row, 0);
        emit dataChanged(modelIndex, modelIndex, {LastMessageContentRole, LastMessageTimeRole});
    }
}

void ChatListModel::updateUnreadCount(qint64 conversationId, int count)
{
    if (!m_conversations.contains(conversationId))
        return;

    Conversation &conversation = m_conversations[conversationId];
    conversation.unreadCount = count;

    int row = m_conversationIds.indexOf(conversationId);
    if (row != -1) {
        QModelIndex modelIndex = createIndex(row, 0);
        emit dataChanged(modelIndex, modelIndex, {UnreadCountRole});
    }
}

void ChatListModel::updateTopStatus(qint64 conversationId, bool isTop)
{
    if (!m_conversations.contains(conversationId))
        return;

    Conversation &conversation = m_conversations[conversationId];
    conversation.isTop = isTop;

    int row = m_conversationIds.indexOf(conversationId);
    if (row != -1) {
        QModelIndex modelIndex = createIndex(row, 0);
        emit dataChanged(modelIndex, modelIndex, {IsTopRole});
    }
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
        if (conv.userId == contactId || conv.groupId == contactId) {
            return createIndex(row, 0);
        }
    }
    return QModelIndex();
}
