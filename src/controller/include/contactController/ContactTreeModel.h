#ifndef CONTACTTREEMODEL_H
#define CONTACTTREEMODEL_H

#include <QStandardItemModel>
#include <QHash>
#include "Contact.h"
#include "FriendRequest.h"

class ContactTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit ContactTreeModel(QObject *parent = nullptr);

    void setupFixedNodes();
    void loadContacts(const QList<Contact>& contacts);
    void addContact(const Contact& contact);

    // 判断节点类型
    bool isParentNode(const QModelIndex &index) const;
    bool isContactNode(const QModelIndex &index) const;

    // 快速查找（基于哈希表）
    QModelIndex findContactIndex(qint64 userId) const;
    Contact getContactById(qint64 userId) const;
    Contact getCurrentLoginUser();
    void removeContact(qint64 userId);

public:
    // 加载所有好友请求
    void loadFriendRequests(const QList<FriendRequest>& requests);
    // 新增一条请求
    void addFriendRequest(const FriendRequest& request);
    // 删除请求（根据ID）
    void removeFriendRequest(qint64 requestId);
    // 更新请求状态（如已读、已处理）
    void updateFriendRequest(const FriendRequest& request);
    // 判断索引是否为“好友请求”子项
    bool isFriendRequestNode(const QModelIndex& index) const;
    // 获取索引对应的 FriendRequest
    FriendRequest getFriendRequestByIndex(const QModelIndex& index) const;
private:
    QStandardItem *m_newFriendItem;
    QStandardItem *m_officialAccountItem;
    QStandardItem *m_serviceAccountItem;
    QStandardItem *m_contactGroupItem;

    // 哈希表：userId -> QStandardItem*（指向联系人子项）
    QHash<qint64, QStandardItem*> m_contactItems;
    // requestId → item
    QHash<qint64, QStandardItem*> m_friendRequestItems;

};

#endif // CONTACTTREEMODEL_H
