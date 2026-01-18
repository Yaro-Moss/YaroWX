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
    return m_messages.size();
}

QVariant ChatMessagesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.size())
        return QVariant();

    const Message &message = m_messages.at(index.row());

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
    case Qt::DisplayRole:
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

    return QVariant();
}

bool ChatMessagesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_messages.size())
        return false;

    Message &message = m_messages[index.row()];

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
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(message);
    endInsertRows();
}

void ChatMessagesModel::insertMessage(int row, const Message &message)
{
    if (row < 0 || row > m_messages.size()) return;

    beginInsertRows(QModelIndex(), row, row);
    m_messages.insert(row, message);
    endInsertRows();
}

void ChatMessagesModel::removeMessage(int row)
{
    if (row < 0 || row >= m_messages.size()) return;

    beginRemoveRows(QModelIndex(), row, row);
    m_messages.removeAt(row);
    endRemoveRows();
}

void ChatMessagesModel::removeMessageById(qint64 messageId)
{
    int index = findMessageIndexById(messageId);
    if (index != -1) {
        removeMessage(index);
    }
}

void ChatMessagesModel::updateMessage(const Message &message)
{
    int index = findMessageIndexById(message.messageId);
    if (index != -1) {
        m_messages[index] = message;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);
    }
}

Message ChatMessagesModel::getMessage(int row) const
{
    if (row >= 0 && row < m_messages.size())
        return m_messages.at(row);
    return Message();
}

Message ChatMessagesModel::getMessageById(qint64 messageId) const
{
    int index = findMessageIndexById(messageId);
    if (index != -1) {
        return m_messages.at(index);
    }
    return Message();
}

void ChatMessagesModel::addMessages(const QVector<Message> &messages)
{
    if (messages.isEmpty()) return;

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size() + messages.size() - 1);
    m_messages.append(messages);
    endInsertRows();
}

void ChatMessagesModel::clearAll()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

int ChatMessagesModel::findMessageIndexById(qint64 messageId) const
{
    for (int i = 0; i < m_messages.size(); ++i) {
        if (m_messages.at(i).messageId == messageId) {
            return i;
        }
    }
    return -1;
}

bool ChatMessagesModel::containsMessage(qint64 messageId) const
{
    return findMessageIndexById(messageId) != -1;
}

void ChatMessagesModel::setCurrentUserId(qint64 userId)
{
    if (m_currentUserId != userId) {
        m_currentUserId = userId;
        // 刷新所有消息的isOwn状态
        if (!m_messages.isEmpty()) {
            emit dataChanged(createIndex(0, 0), createIndex(m_messages.size() - 1, 0), {IsOwnRole});
        }
    }
}

qint64 ChatMessagesModel::currentUserId() const
{
    return m_currentUserId;
}

void ChatMessagesModel::setConversationId(qint64 conversationId)
{
    m_currentConversationId = conversationId;
}

qint64 ChatMessagesModel::conversationId() const
{
    return m_currentConversationId;
}
