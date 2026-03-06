#include "ContactController.h"
#include <QDebug>
#include "ContactTable.h"

ContactController::ContactController(DatabaseManager* dbManager, QObject* parent)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_contactTable(nullptr)
    , m_contactTreeModel(new ContactTreeModel(this))
    , m_reqIdCounter(0)
{
    if (m_dbManager) {
        m_contactTable = m_dbManager->contactTable();
        connectSignals();
        connectAsyncSignals(); // 连接异步信号
    } else {
        qWarning() << "DatabaseManager is null in ContactController constructor";
    }
}

ContactController::~ContactController()
{
    disconnectSignals();
}

void ContactController::connectSignals()
{
    if (!m_contactTable) return;

    // 连接数据库操作结果信号
    connect(m_contactTable, &ContactTable::contactSaved,
            this, &ContactController::onContactSaved);
    connect(m_contactTable, &ContactTable::contactUpdated,
            this, &ContactController::onContactUpdated);
    connect(m_contactTable, &ContactTable::contactDeleted,
            this, &ContactController::onContactDeleted);
    connect(m_contactTable, &ContactTable::contactLoaded,
            this, &ContactController::onContactLoaded);
    connect(m_contactTable, &ContactTable::allContactsLoaded,
            this, &ContactController::onAllContactsLoaded);
    connect(m_contactTable, &ContactTable::searchContactsResult,
            this, &ContactController::onSearchContactsResult);
    connect(m_contactTable, &ContactTable::contactStarredSet,
            this, &ContactController::onContactStarredSet);
    connect(m_contactTable, &ContactTable::contactBlockedSet,
            this, &ContactController::onContactBlockedSet);
    connect(m_contactTable, &ContactTable::starredContactsLoaded,
            this, &ContactController::onStarredContactsLoaded);
}

void ContactController::connectAsyncSignals()
{
    if (!m_contactTable) return;

    // 直接连接到 ContactTable 的方法，使用异步连接
    connect(this, &ContactController::addContactRequested,
            m_contactTable, &ContactTable::saveContact, Qt::QueuedConnection);
    connect(this, &ContactController::updateContactRequested,
            m_contactTable, &ContactTable::updateContact, Qt::QueuedConnection);
    connect(this, &ContactController::deleteContactRequested,
            m_contactTable, &ContactTable::deleteContact, Qt::QueuedConnection);
    connect(this, &ContactController::getContactRequested,
            m_contactTable, &ContactTable::getContact, Qt::QueuedConnection);
    connect(this, &ContactController::getAllContactsRequested,
            m_contactTable, &ContactTable::getAllContacts, Qt::QueuedConnection);
    connect(this, &ContactController::searchContactsRequested,
            m_contactTable, &ContactTable::searchContacts, Qt::QueuedConnection);
    connect(this, &ContactController::setContactStarredRequested,
            m_contactTable, &ContactTable::setContactStarred, Qt::QueuedConnection);
    connect(this, &ContactController::setContactBlockedRequested,
            m_contactTable, &ContactTable::setContactBlocked, Qt::QueuedConnection);
    connect(this, &ContactController::getStarredContactsRequested,
            m_contactTable, &ContactTable::getStarredContacts, Qt::QueuedConnection);
    connect(this, &ContactController::getCurrentUserRequested,
            m_contactTable, &ContactTable::getCurrentUser, Qt::QueuedConnection);
}

void ContactController::disconnectSignals()
{
    if (!m_contactTable) return;

    disconnect(m_contactTable, &ContactTable::contactSaved,
               this, &ContactController::onContactSaved);
    disconnect(m_contactTable, &ContactTable::contactUpdated,
               this, &ContactController::onContactUpdated);
    disconnect(m_contactTable, &ContactTable::contactDeleted,
               this, &ContactController::onContactDeleted);
    disconnect(m_contactTable, &ContactTable::contactLoaded,
               this, &ContactController::onContactLoaded);
    disconnect(m_contactTable, &ContactTable::allContactsLoaded,
               this, &ContactController::onAllContactsLoaded);
    disconnect(m_contactTable, &ContactTable::searchContactsResult,
               this, &ContactController::onSearchContactsResult);
    disconnect(m_contactTable, &ContactTable::contactStarredSet,
               this, &ContactController::onContactStarredSet);
    disconnect(m_contactTable, &ContactTable::contactBlockedSet,
               this, &ContactController::onContactBlockedSet);
    disconnect(m_contactTable, &ContactTable::starredContactsLoaded,
               this, &ContactController::onStarredContactsLoaded);

    // 断开异步信号连接
    disconnect(this, &ContactController::addContactRequested,
               m_contactTable, &ContactTable::saveContact);
    disconnect(this, &ContactController::updateContactRequested,
               m_contactTable, &ContactTable::updateContact);
    disconnect(this, &ContactController::deleteContactRequested,
               m_contactTable, &ContactTable::deleteContact);
    disconnect(this, &ContactController::getContactRequested,
               m_contactTable, &ContactTable::getContact);
    disconnect(this, &ContactController::getAllContactsRequested,
               m_contactTable, &ContactTable::getAllContacts);
    disconnect(this, &ContactController::searchContactsRequested,
               m_contactTable, &ContactTable::searchContacts);
    disconnect(this, &ContactController::setContactStarredRequested,
               m_contactTable, &ContactTable::setContactStarred);
    disconnect(this, &ContactController::setContactBlockedRequested,
               m_contactTable, &ContactTable::setContactBlocked);
    disconnect(this, &ContactController::getStarredContactsRequested,
               m_contactTable, &ContactTable::getStarredContacts);
}

int ContactController::generateReqId()
{
    return m_reqIdCounter.fetchAndAddAcquire(1);
}


// 异步操作方法 - 只进行参数验证和发射信号
void ContactController::addContact(int reqId, const Contact& contact)
{
    if (!contact.isValid()) {
        emit contactAdded(reqId, false, "Invalid contact data");
        return;
    }

    if (!m_contactTable) {
        emit contactAdded(reqId, false, "Contact table not available");
        return;
    }

    emit addContactRequested(reqId, contact);
}

void ContactController::updateContact(int reqId, const Contact& contact)
{
    if (!contact.isValid()) {
        emit contactUpdated(reqId, false, "Invalid contact data");
        return;
    }

    if (!m_contactTable) {
        emit contactUpdated(reqId, false, "Contact table not available");
        return;
    }

    emit updateContactRequested(reqId, contact);
}

void ContactController::deleteContact(int reqId, qint64 userId)
{
    if (!m_contactTable) {
        emit contactDeleted(reqId, false, "Contact table not available");
        return;
    }

    emit deleteContactRequested(reqId, userId);
}


void ContactController::getCurrentUser()
{
    if (!m_contactTable) {
        return;
    }
    int reqId = generateReqId();

    qDebug()<<"获取当前用户";
    m_pendingUpdates[reqId] = "getCurrentUser";
    emit getCurrentUserRequested(reqId);
}

void ContactController::getContact(int reqId, qint64 userId)
{
    if (!m_contactTable) {
        emit contactLoaded(reqId, Contact());
        return;
    }

    emit getContactRequested(reqId, userId);
}

void ContactController::getAllContacts(int reqId)
{
    if (!m_contactTable) {
        emit allContactsLoaded(reqId, QList<Contact>());
        return;
    }

    emit getAllContactsRequested(reqId);
}

void ContactController::searchContacts(int reqId, const QString& keyword)
{
    if (!m_contactTable) {
        emit searchContactsResult(reqId, QList<Contact>());
        return;
    }

    emit searchContactsRequested(reqId, keyword);
}

void ContactController::setContactStarred(int reqId, qint64 userId, bool starred)
{
    if (!m_contactTable) {
        emit contactStarredChanged(reqId, false, "Contact table not available");
        return;
    }

    emit setContactStarredRequested(reqId, userId, starred);
}

void ContactController::setContactBlocked(int reqId, qint64 userId, bool blocked)
{
    if (!m_contactTable) {
        emit contactBlockedChanged(reqId, false, "Contact table not available");
        return;
    }

    emit setContactBlockedRequested(reqId, userId, blocked);
}


void ContactController::setSelectedContact(const Contact &contact)
{
    m_contact = contact;
}


void ContactController::getStarredContacts(int reqId)
{
    if (!m_contactTable) {
        emit starredContactsLoaded(reqId, QList<Contact>());
        return;
    }

    emit getStarredContactsRequested(reqId);
}

// 数据库操作结果处理槽函数
void ContactController::onContactSaved(int reqId, bool success, const QString& error)
{
    if (success) {
        emit contactsChanged();
    }
    emit contactAdded(reqId, success, error);
}

void ContactController::onContactUpdated(int reqId, bool success, const QString& error)
{
    if (success) {
        emit contactsChanged();
    }
    emit contactUpdated(reqId, success, error);
}

void ContactController::onContactDeleted(int reqId, bool success, const QString& error)
{
    if (success) {
        getAllContacts(reqId);
        emit contactsChanged();
    }
    emit contactDeleted(reqId, success, error);
}

void ContactController::onContactLoaded(int reqId, const Contact& contact)
{
    // 检查是否是更新操作的中间步骤
    if (m_pendingUpdates.contains(reqId)) {
        if(m_pendingUpdates[reqId]=="getCurrentUser")
            emit currentUserLoaded(reqId, contact);

    } else {
        emit contactLoaded(reqId, contact);
    }
}

void ContactController::onAllContactsLoaded(int reqId, const QList<Contact>& contacts)
{
    m_contactTreeModel->loadContacts(contacts);
    emit allContactsLoaded(reqId, contacts);
}

void ContactController::onSearchContactsResult(int reqId, const QList<Contact>& contacts)
{
    emit searchContactsResult(reqId, contacts);
}

void ContactController::onContactStarredSet(int reqId, bool success)
{
    if (success) {
        getAllContacts(reqId);
        emit contactsChanged();
    }
    emit contactStarredChanged(reqId, success, success ? QString() : "Failed to set starred");
}

void ContactController::onContactBlockedSet(int reqId, bool success)
{
    if (success) {
        emit contactsChanged();
    }
    emit contactBlockedChanged(reqId, success, success ? QString() : "Failed to set blocked");
}

void ContactController::onStarredContactsLoaded(int reqId, const QList<Contact>& contacts)
{
    emit starredContactsLoaded(reqId, contacts);
}

Contact ContactController::getContactFromModel(qint64 userId)
{
    return m_contactTreeModel->getContactById(userId);
}

