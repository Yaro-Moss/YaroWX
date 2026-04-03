#include "ContactDao.h"
#include "DbConnectionManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "ORM.h"

ContactDao::ContactDao()
{
}

QList<Contact> ContactDao::fetchAllContacts()
{
    QList<Contact> contacts;
    auto dbPtr = DbConnectionManager::connectionForCurrentThread();
    if (!dbPtr || !dbPtr->isOpen()) {
        qWarning() << "ContactDao::fetchAllContacts: no database connection";
        return contacts;
    }
    QSqlDatabase db = *dbPtr;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            c.user_id,
            c.remark_name,
            c.description,
            c.tags,
            c.phone_note,
            c.email_note,
            c.source,
            c.is_starred,
            c.is_blocked,
            c.add_time,
            u.account,
            u.nickname,
            u.avatar,
            u.avatar_local_path,
            u.profile_cover,
            u.gender,
            u.region,
            u.signature,
            u.is_current
        FROM contacts c
        INNER JOIN users u ON c.user_id = u.user_id
        ORDER BY c.remark_name, u.nickname
    )");
    if (!query.exec()) {
        qWarning() << "ContactDao::fetchAllContacts query failed:" << query.lastError().text();
        return contacts;
    }

    while (query.next()) {
        Contact contact;
        contact.setuser_id(query.value("user_id").toLongLong());
        contact.setremark_name(query.value("remark_name").toString());
        contact.setdescription(query.value("description").toString());
        contact.settags(query.value("tags").toString());
        contact.setphone_note(query.value("phone_note").toString());
        contact.setemail_note(query.value("email_note").toString());
        contact.setsource(query.value("source").toString());
        contact.setis_starred(query.value("is_starred").toInt());
        contact.setis_blocked(query.value("is_blocked").toInt());
        contact.setadd_time(query.value("add_time").toLongLong());

        User user;
        user.setuser_id(query.value("user_id").toLongLong());
        user.setaccount(query.value("account").toString());
        user.setnickname(query.value("nickname").toString());
        user.setavatar(query.value("avatar").toString());
        user.setavatar_local_path(query.value("avatar_local_path").toString());
        user.setprofile_cover(query.value("profile_cover").toString());
        user.setgender(query.value("gender").toInt());
        user.setregion(query.value("region").toString());
        user.setsignature(query.value("signature").toString());
        user.setis_current(query.value("is_current").toInt());
        contact.user = user;

        contacts.append(contact);
    }
    return contacts;
}

Contact ContactDao::fetchContactById(qint64 userId)
{
    Contact contact;
    auto dbPtr = DbConnectionManager::connectionForCurrentThread();
    if (!dbPtr || !dbPtr->isOpen()) {
        qWarning() << "ContactDao::fetchContactById: no database connection";
        return contact;
    }
    QSqlDatabase db = *dbPtr;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            c.user_id,
            c.remark_name,
            c.description,
            c.tags,
            c.phone_note,
            c.email_note,
            c.source,
            c.is_starred,
            c.is_blocked,
            c.add_time,
            u.account,
            u.nickname,
            u.avatar,
            u.avatar_local_path,
            u.profile_cover,
            u.gender,
            u.region,
            u.signature,
            u.is_current
        FROM contacts c
        INNER JOIN users u ON c.user_id = u.user_id
        WHERE c.user_id = ?
    )");
    query.addBindValue(userId);
    if (!query.exec() || !query.next()) {
        qWarning() << "ContactDao::fetchContactById failed:" << query.lastError().text();
        return contact;
    }

    contact.setuser_id(query.value("user_id").toLongLong());
    contact.setremark_name(query.value("remark_name").toString());
    contact.setdescription(query.value("description").toString());
    contact.settags(query.value("tags").toString());
    contact.setphone_note(query.value("phone_note").toString());
    contact.setemail_note(query.value("email_note").toString());
    contact.setsource(query.value("source").toString());
    contact.setis_starred(query.value("is_starred").toInt());
    contact.setis_blocked(query.value("is_blocked").toInt());
    contact.setadd_time(query.value("add_time").toLongLong());

    User user;
    user.setuser_id(query.value("user_id").toLongLong());
    user.setaccount(query.value("account").toString());
    user.setnickname(query.value("nickname").toString());
    user.setavatar(query.value("avatar").toString());
    user.setavatar_local_path(query.value("avatar_local_path").toString());
    user.setprofile_cover(query.value("profile_cover").toString());
    user.setgender(query.value("gender").toInt());
    user.setregion(query.value("region").toString());
    user.setsignature(query.value("signature").toString());
    user.setis_current(query.value("is_current").toInt());
    contact.user = user;

    return contact;
}

QList<Contact> ContactDao::searchContacts(const QString& keyword)
{
    QList<Contact> contacts;
    auto dbPtr = DbConnectionManager::connectionForCurrentThread();
    if (!dbPtr || !dbPtr->isOpen()) {
        qWarning() << "ContactDao::searchContacts: no database connection";
        return contacts;
    }
    QSqlDatabase db = *dbPtr;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            c.user_id,
            c.remark_name,
            c.description,
            c.tags,
            c.phone_note,
            c.email_note,
            c.source,
            c.is_starred,
            c.is_blocked,
            c.add_time,
            u.account,
            u.nickname,
            u.avatar,
            u.avatar_local_path,
            u.profile_cover,
            u.gender,
            u.region,
            u.signature,
            u.is_current
        FROM contacts c
        INNER JOIN users u ON c.user_id = u.user_id
        WHERE u.account = ?  -- 只查：账号 完全相等
        ORDER BY c.remark_name, u.nickname
    )");

    // 直接绑定关键词，不加 %！！！这是精准查询
    query.addBindValue(keyword);

    if (!query.exec()) {
        qWarning() << "ContactDao::searchContacts query failed:" << query.lastError().text();
        return contacts;
    }
    while (query.next()) {
        Contact contact;
        contact.setuser_id(query.value("user_id").toLongLong());
        contact.setremark_name(query.value("remark_name").toString());
        contact.setdescription(query.value("description").toString());
        contact.settags(query.value("tags").toString());
        contact.setphone_note(query.value("phone_note").toString());
        contact.setemail_note(query.value("email_note").toString());
        contact.setsource(query.value("source").toString());
        contact.setis_starred(query.value("is_starred").toInt());
        contact.setis_blocked(query.value("is_blocked").toInt());
        contact.setadd_time(query.value("add_time").toLongLong());

        User user;
        user.setuser_id(query.value("user_id").toLongLong());
        user.setaccount(query.value("account").toString());
        user.setnickname(query.value("nickname").toString());
        user.setavatar(query.value("avatar").toString());
        user.setavatar_local_path(query.value("avatar_local_path").toString());
        user.setprofile_cover(query.value("profile_cover").toString());
        user.setgender(query.value("gender").toInt());
        user.setregion(query.value("region").toString());
        user.setsignature(query.value("signature").toString());
        user.setis_current(query.value("is_current").toInt());
        contact.user = user;

        contacts.append(contact);
    }
    return contacts;
}

bool ContactDao::updateContact(const Contact& contact)
{
    Orm orm;
    return orm.update(contact);
}

bool ContactDao::removeContact(qint64 userId)
{
    Orm orm;
    Contact dummy;
    dummy.setuser_id(userId);
    return orm.remove(dummy);
}

bool ContactDao::addContact(const Contact& contact)
{
    Orm orm;
    Contact copy = contact;
    return orm.insert(copy);
}