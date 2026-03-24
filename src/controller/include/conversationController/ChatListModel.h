#ifndef CHATLISTMODEL_H
#define CHATLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include "Conversation.h"

class ChatListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ChatListModel(QObject *parent = nullptr);

    // QAbstractListModel 接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    // 会话管理
    void loadConversations(const QList<Conversation>& conversations);
    void addConversation(const Conversation &conversation);
    void updateConversation(const Conversation &conversation);
    void updateLastMessage(qint64 conversationId, const QString &message, qint64 time);
    void updateUnreadCount(qint64 conversationId, int count);
    void updateTopStatus(qint64 conversationId, bool isTop);
    void removeConversation(qint64 conversationId);
    void clearAll();

    // 查询方法
    Conversation getConversation(qint64 conversationId) const;
    Conversation getConversationAt(int index) const;
    int findConversationIndex(qint64 conversationId) const;

    QModelIndex getConversationIndex(qint64 conversationId) const;
    QModelIndex getConversationIndexByContactId(qint64 contactId) const;

private:
    static bool lessThan(const Conversation &a, const Conversation &b) {
        // 置顶的排在前面
        if (a.is_top() != b.is_top())
            return a.is_topValue() > b.is_topValue();
        // 按最后消息时间倒序（新的在前）
        return a.last_message_timeValue() > b.last_message_timeValue();
    }

    void reorderConversation(qint64 id);          // 根据当前数据将指定会话移到正确位置
    int findInsertPosition(qint64 id) const;      // 计算指定会话应处的位置（索引）

    QHash<qint64, Conversation> m_conversations;   // 会话数据缓存（通过ID快速访问）
    QList<qint64> m_conversationIds;                // 按显示顺序存储的会话ID列表
};

#endif // CHATLISTMODEL_H
