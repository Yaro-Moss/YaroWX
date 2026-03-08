#include "ChatMessagesModel.h"
#include <QDebug>
#include "FormatTime.h"

ChatMessagesModel::ChatMessagesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ChatMessagesModel::ChatMessagesModel(int currentUserId, QObject *parent)
    : QAbstractListModel(parent)
    , m_currentUserId(currentUserId)
{
}

int ChatMessagesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_messageIds.size();
}

QVariant ChatMessagesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messageIds.size())
        return QVariant();

    qint64 id = m_messageIds.at(index.row());
    const Message &message = m_messages.value(id);

    switch (role) {
    case MessageIdRole:
        return message.messageId;
    case ConversationIdRole:
        return message.conversationId;
    case SenderIdRole:
        return message.senderId;
    case TypeRole:
        return static_cast<int>(message.type);
    case ContentRole:
        return message.content;
    case FilePathRole:
        return message.filePath;
    case FileUrlRole:
        return message.fileUrl;
    case FileSizeRole:
        return message.fileSize;
    case DurationRole:
        return message.duration;
    case ThumbnailPathRole:
        return message.thumbnailPath;
    case TimestampRole:
        return message.timestamp;
    case SenderNameRole:
        return message.senderName;
    case AvatarRole:
        return message.avatar;
    case IsOwnRole:
        return message.isOwn(m_currentUserId);
    case IsTextRole:
        return message.isText();
    case IsImageRole:
        return message.isImage();
    case IsVideoRole:
        return message.isVideo();
    case IsFileRole:
        return message.isFile();
    case IsVoiceRole:
        return message.isVoice();
    case IsMediaRole:
        return message.isMedia();
    case HasFileRole:
        return message.hasFile();
    case HasThumbnailRole:
        return message.hasThumbnail();
    case FormattedFileSizeRole:
        return message.formattedFileSize();
    case FormattedDurationRole:
        return message.formattedDuration();
    case FullMessageRole:
        return QVariant::fromValue(message);
    case Qt::DisplayRole: {
        // 显示用文本预览
        QString preview;
        switch (message.type) {
        case MessageType::TEXT:
            preview = message.content;
            break;
        case MessageType::IMAGE:
            preview = "[图片] " + message.content;
            break;
        case MessageType::VIDEO:
            preview = "[视频] " + message.content;
            break;
        case MessageType::FILE:
            preview = "[文件] " + message.content + " (" + message.formattedFileSize() + ")";
            break;
        case MessageType::VOICE:
            preview = "[语音] " + message.formattedDuration();
            break;
        }
        return QString("[%1] %2: %3").arg(
            FormatTime(message.timestamp),
            message.senderName,
            preview.left(50)
            );
    }
    default:
        return QVariant();
    }
}

bool ChatMessagesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_messageIds.size())
        return false;

    qint64 id = m_messageIds.at(index.row());
    Message &message = m_messages[id];

    switch (role) {
    case ContentRole:
        message.content = value.toString();
        break;
    case SenderNameRole:
        message.senderName = value.toString();
        break;
    case AvatarRole:
        message.avatar = value.toString();
        break;
    case FilePathRole:
        message.filePath = value.toString();
        break;
    case FileUrlRole:
        message.fileUrl = value.toString();
        break;
    case FileSizeRole:
        message.fileSize = value.toLongLong();
        break;
    case DurationRole:
        message.duration = value.toInt();
        break;
    case ThumbnailPathRole:
        message.thumbnailPath = value.toString();
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, {role});
    return true;
}

QHash<int, QByteArray> ChatMessagesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MessageIdRole] = "messageId";
    roles[ConversationIdRole] = "conversationId";
    roles[SenderIdRole] = "senderId";
    roles[TypeRole] = "type";
    roles[ContentRole] = "content";
    roles[FilePathRole] = "filePath";
    roles[FileUrlRole] = "fileUrl";
    roles[FileSizeRole] = "fileSize";
    roles[DurationRole] = "duration";
    roles[ThumbnailPathRole] = "thumbnailPath";
    roles[TimestampRole] = "timestamp";
    roles[SenderNameRole] = "senderName";
    roles[AvatarRole] = "avatar";
    roles[IsOwnRole] = "isOwn";
    roles[IsTextRole] = "isText";
    roles[IsImageRole] = "isImage";
    roles[IsVideoRole] = "isVideo";
    roles[IsFileRole] = "isFile";
    roles[IsVoiceRole] = "isVoice";
    roles[IsMediaRole] = "isMedia";
    roles[HasFileRole] = "hasFile";
    roles[HasThumbnailRole] = "hasThumbnail";
    roles[FormattedFileSizeRole] = "formattedFileSize";
    roles[FormattedDurationRole] = "formattedDuration";
    roles[FullMessageRole] = "fullMessage";
    return roles;
}

void ChatMessagesModel::addMessage(const Message &message)
{
    qint64 id = message.messageId;
    if (m_messages.contains(id)) {
        // 已存在：更新数据，位置不变
        m_messages[id] = message;
        int row = m_messageIds.indexOf(id);
        if (row != -1) {
            QModelIndex idx = createIndex(row, 0);
            emit dataChanged(idx, idx);
        }
        return;
    }

    // 新消息：追加到末尾（假设外部保证顺序）
    beginInsertRows(QModelIndex(), m_messageIds.size(), m_messageIds.size());
    m_messages.insert(id, message);
    m_messageIds.append(id);
    endInsertRows();
}

void ChatMessagesModel::insertMessage(int row, const Message &message)
{
    if (row < 0 || row > m_messageIds.size())
        return;

    qint64 id = message.messageId;
    if (m_messages.contains(id)) {
        // 已存在：可能需要移动位置
        int oldRow = m_messageIds.indexOf(id);
        if (oldRow == row) {
            // 位置相同，只更新数据
            m_messages[id] = message;
            QModelIndex idx = createIndex(row, 0);
            emit dataChanged(idx, idx);
        } else {
            // 位置不同：先更新数据，再移动行
            m_messages[id] = message;
            if (beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), row > oldRow ? row + 1 : row)) {
                m_messageIds.move(oldRow, row);
                endMoveRows();
            }
        }
        return;
    }

    // 新消息：插入指定位置
    beginInsertRows(QModelIndex(), row, row);
    m_messages.insert(id, message);
    m_messageIds.insert(row, id);
    endInsertRows();
}

void ChatMessagesModel::removeMessage(int row)
{
    if (row < 0 || row >= m_messageIds.size())
        return;

    beginRemoveRows(QModelIndex(), row, row);
    qint64 id = m_messageIds.at(row);
    m_messages.remove(id);
    m_messageIds.removeAt(row);
    endRemoveRows();
}

void ChatMessagesModel::removeMessageById(qint64 messageId)
{
    int row = m_messageIds.indexOf(messageId);
    if (row != -1)
        removeMessage(row);
}

void ChatMessagesModel::updateMessage(const Message &message)
{
    qint64 id = message.messageId;
    if (!m_messages.contains(id))
        return;

    m_messages[id] = message;
    int row = m_messageIds.indexOf(id);
    if (row != -1) {
        QModelIndex idx = createIndex(row, 0);
        emit dataChanged(idx, idx);
    }
}

Message ChatMessagesModel::getMessage(int row) const
{
    if (row >= 0 && row < m_messageIds.size()) {
        qint64 id = m_messageIds.at(row);
        return m_messages.value(id);
    }
    return Message();
}

Message ChatMessagesModel::getMessageById(qint64 messageId) const
{
    return m_messages.value(messageId);
}

void ChatMessagesModel::addMessages(const QVector<Message> &messages)
{
    if (messages.isEmpty())
        return;

    // 批量添加：过滤出确实不存在的消息，然后一次性插入末尾
    QVector<Message> newMessages;
    for (const Message &msg : messages) {
        if (!m_messages.contains(msg.messageId)) {
            newMessages.append(msg);
        } else {
            // 已存在的消息单独更新（位置不变）
            updateMessage(msg);
        }
    }

    if (newMessages.isEmpty())
        return;

    int firstRow = m_messageIds.size();
    int lastRow = firstRow + newMessages.size() - 1;
    beginInsertRows(QModelIndex(), firstRow, lastRow);
    for (const Message &msg : newMessages) {
        m_messages.insert(msg.messageId, msg);
        m_messageIds.append(msg.messageId);
    }
    endInsertRows();
}

void ChatMessagesModel::clearAll()
{
    beginResetModel();
    m_messages.clear();
    m_messageIds.clear();
    endResetModel();
}

int ChatMessagesModel::findMessageIndexById(qint64 messageId) const
{
    return m_messageIds.indexOf(messageId);
}

bool ChatMessagesModel::containsMessage(qint64 messageId) const
{
    return m_messages.contains(messageId);
}

QVector<Message> ChatMessagesModel::getRecentMessages(int count) const
{
    QVector<Message> result;
    int total = m_messageIds.size();
    int takeCount = qMin(count, total);
    result.reserve(takeCount);
    // 从消息列表末尾向前取
    for (int i = 0; i < takeCount; ++i) {
        qint64 id = m_messageIds.at(total - 1 - i);
        result.append(m_messages.value(id));
    }
    return result;
}

void ChatMessagesModel::setCurrentUserId(qint64 userId)
{
    if (m_currentUserId != userId) {
        m_currentUserId = userId;
        if (!m_messageIds.isEmpty()) {
            // 刷新所有消息的 isOwn 状态
            emit dataChanged(createIndex(0, 0), createIndex(m_messageIds.size() - 1, 0), {IsOwnRole});
        }
        emit currentUserIdChanged();
    }
}

qint64 ChatMessagesModel::currentUserId() const
{
    return m_currentUserId;
}

void ChatMessagesModel::setConversationId(qint64 conversationId)
{
    if (m_currentConversationId != conversationId) {
        m_currentConversationId = conversationId;
        emit conversationIdChanged();
    }
}

qint64 ChatMessagesModel::conversationId() const
{
    return m_currentConversationId;
}
