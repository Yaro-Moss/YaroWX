#include "ContactController.h"
#include "ContactDao.h"
#include "ORM.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

ContactController::ContactController(QObject* parent)
    : QObject(parent)
    , m_contactTreeModel(new ContactTreeModel(this))
{
}

ContactController::~ContactController()
{
}

void ContactController::getAllContacts()
{
    QFutureWatcher<QList<Contact>> *watcher = new QFutureWatcher<QList<Contact>>(this);
    connect(watcher, &QFutureWatcher<QList<Contact>>::finished, this, [this, watcher]() {
        QList<Contact> contacts = watcher->result();
        m_contactTreeModel->loadContacts(contacts);
        emit contactsLoaded();
        watcher->deleteLater();
    });

    QFuture<QList<Contact>> future = QtConcurrent::run([]() -> QList<Contact> {
        ContactDao dao;
        return dao.fetchAllContacts();
    });
    watcher->setFuture(future);
}

void ContactController::searchContacts(const QString& keyword)
{
    if (keyword.trimmed().isEmpty()) {
        getAllContacts();
        return;
    }

    QFutureWatcher<QList<Contact>> *watcher = new QFutureWatcher<QList<Contact>>(this);
    connect(watcher, &QFutureWatcher<QList<Contact>>::finished, this, [this, watcher]() {
        QList<Contact> contacts = watcher->result();
        m_contactTreeModel->loadContacts(contacts);
        emit contactsLoaded();
        watcher->deleteLater();
    });

    QFuture<QList<Contact>> future = QtConcurrent::run([keyword]() -> QList<Contact> {
        ContactDao dao;
        return dao.searchContacts(keyword);
    });
    watcher->setFuture(future);
}

void ContactController::getContact(qint64 userId)
{
    QFutureWatcher<Contact> *watcher = new QFutureWatcher<Contact>(this);
    connect(watcher, &QFutureWatcher<Contact>::finished, this, [this, watcher, userId]() {
        Contact contact = watcher->result();
        if (contact.isValid()) {
            m_contactTreeModel->addContact(contact);  // 更新模型（addContact 会更新或添加）
            emit contactUpdated(userId);
        }
        watcher->deleteLater();
    });

    QFuture<Contact> future = QtConcurrent::run([userId]() -> Contact {
        ContactDao dao;
        return dao.fetchContactById(userId);
    });
    watcher->setFuture(future);
}

void ContactController::addContact(const Contact& contact)
{
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, contact]() {
        if (watcher->result()) {
            m_contactTreeModel->addContact(contact);
            emit contactUpdated(contact.user_idValue());
        } else {
            qWarning() << "Add contact failed";
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([contact]() -> bool {
        ContactDao dao;
        return dao.addContact(contact);
    });
    watcher->setFuture(future);
}

void ContactController::updateContact(const Contact& contact)
{
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, contact]() {
        if (watcher->result()) {
            m_contactTreeModel->addContact(contact);  // 更新模型
            emit contactUpdated(contact.user_idValue());
        } else {
            qWarning() << "Update contact failed";
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([contact]() -> bool {
        ContactDao dao;
        return dao.updateContact(contact);
    });
    watcher->setFuture(future);
}

void ContactController::deleteContact(qint64 userId)
{
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, userId]() {
        if (watcher->result()) {
            m_contactTreeModel->removeContact(userId);
            emit contactRemoved(userId);
        } else {
            qWarning() << "Delete contact failed";
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([userId]() -> bool {
        ContactDao dao;
        return dao.removeContact(userId);
    });
    watcher->setFuture(future);
}

void ContactController::setContactStarred(qint64 userId, bool starred)
{
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, userId, starred]() {
        if (watcher->result()) {
            getContact(userId);
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([userId, starred]() -> bool {
        Orm orm;
        auto contactOpt = orm.findById<Contact>(userId);
        if (!contactOpt) return false;
        Contact contact = *contactOpt;
        contact.setis_starred(starred ? 1 : 0);
        return orm.update(contact);
    });
    watcher->setFuture(future);
}

void ContactController::setContactBlocked(qint64 userId, bool blocked)
{
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, userId, blocked]() {
        if (watcher->result()) {
            getContact(userId);
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([userId, blocked]() -> bool {
        Orm orm;
        auto contactOpt = orm.findById<Contact>(userId);
        if (!contactOpt) return false;
        Contact contact = *contactOpt;
        contact.setis_blocked(blocked ? 1 : 0);
        return orm.update(contact);
    });
    watcher->setFuture(future);
}

Contact ContactController::getContactFromModel(qint64 userId)
{
    return m_contactTreeModel->getContactById(userId);
}

void ContactController::setSelectedContact(const Contact &contact)
{
    m_selectedContact = contact;
}

Contact ContactController::getCurrentLoginUser()
{
    return m_contactTreeModel->getCurrentLoginUser();
}
