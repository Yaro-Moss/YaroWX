#ifndef CONTACTTREEMODEL_H
#define CONTACTTREEMODEL_H

#include <QStandardItemModel>
#include <QHash>
#include "Contact.h"

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

private:
    QStandardItem *m_newFriendItem;
    QStandardItem *m_officialAccountItem;
    QStandardItem *m_serviceAccountItem;
    QStandardItem *m_contactGroupItem;

    // 哈希表：userId -> QStandardItem*（指向联系人子项）
    QHash<qint64, QStandardItem*> m_contactItems;
};

#endif // CONTACTTREEMODEL_H
