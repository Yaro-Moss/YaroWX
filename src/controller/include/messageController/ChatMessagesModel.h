#ifndef CHATMESSAGESMODEL_H
#define CHATMESSAGESMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QVector>
#include "Contact.h"
#include "Message.h"

class ChatMessagesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(qint64 conversationId READ conversationId WRITE setConversationId NOTIFY conversationIdChanged)

public:
    explicit ChatMessagesModel(QObject *parent = nullptr);

    enum MessageRoles {
        MessageIdRole = Qt::UserRole + 1,
        ConversationIdRole,
        SenderIdRole,
        TypeRole,
        ContentRole,
        FilePathRole,
        FileUrlRole,
        FileSizeRole,
        DurationRole,
        ThumbnailPathRole,
        TimestampRole,
        SenderNameRole,
        AvatarRole,
        IsOwnRole,
        IsTextRole,
        IsImageRole,
        IsVideoRole,
        IsFileRole,
        IsVoiceRole,
        IsMediaRole,
        HasFileRole,
        HasThumbnailRole,
        FormattedFileSizeRole,
        FormattedDurationRole,
        FullMessageRole
    };

    // QAbstractListModel 接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    // 消息管理
    void addMessage(const Message &message);
    void removeMessage(int row);
    void removeMessageById(qint64 messageId);
    void updateMessage(const Message &message);
    void clearAll();

    // 查询方法
    Message getMessage(int row) const;
    Message getMessageById(qint64 messageId) const;
    int findMessageIndexById(qint64 messageId) const;
    bool containsMessage(qint64 messageId) const;

    // 测试用于当ai提示词
    Q_INVOKABLE QVector<Message> getRecentMessages(int count = 9) const;

    // 属性
    void setCurrentLoginUser(const Contact &user);
    void setConversationId(qint64 conversationId);
    qint64 conversationId() const;

signals:
    void currentUserIdChanged();
    void conversationIdChanged();
    void addMsg(int row);

private:
    static bool lessThan(const Message &a, const Message &b) {
        // 按消息时间升序（旧的在前，新的在后）
        return a.msg_timeValue() < b.msg_timeValue();
    }

    int findInsertPosition(const Message &message) const;

    QHash<qint64, Message> m_messages;       // 消息数据缓存（通过 messageId 快速访问）
    QList<qint64> m_messageIds;               // 按显示顺序存储的消息ID列表
    qint64 m_currentConversationId = 0;
    Contact m_currentLoginUser;
};

#endif // CHATMESSAGESMODEL_H
