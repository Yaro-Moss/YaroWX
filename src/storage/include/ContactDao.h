#ifndef CONTACTDAO_H
#define CONTACTDAO_H

#include <QList>
#include "Contact.h"

class ContactDao
{
public:
    ContactDao();

    // 获取所有联系人（包括关联的用户信息）
    QList<Contact> fetchAllContacts();

    // 根据用户ID获取单个联系人
    Contact fetchContactById(qint64 userId);

    // 根据关键词搜索联系人（昵称、备注名、账号）
    QList<Contact> searchContacts(const QString& keyword);

    // 以下为单表操作（直接使用 ORM）
    bool updateContact(const Contact& contact);
    bool removeContact(qint64 userId);
    bool addContact(const Contact& contact);
};

#endif // CONTACTDAO_H