#ifndef CONTACTTREEMODEL_H
#define CONTACTTREEMODEL_H

#include <QStandardItemModel>
#include "Contact.h"

class ContactTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit ContactTreeModel(QObject *parent = nullptr);

    void setupFixedNodes();
    void loadContacts(const QList<Contact>& contacts);
    void addContact(const Contact& contact);

    // 判断是否为父节点
    bool isParentNode(const QModelIndex &index) const;
    bool isContactNode(const QModelIndex &index) const;

    QModelIndex findContactIndex(qint64 userId) const;
    Contact getContactById(qint64 userId) const;

private:
    QStandardItem *m_newFriendItem;
    QStandardItem *m_officialAccountItem;
    QStandardItem *m_serviceAccountItem;
    QStandardItem *m_contactGroupItem;
};

#endif // CONTACTTREEMODEL_H
