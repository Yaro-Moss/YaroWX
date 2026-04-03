#include "ContactController.h"
#include "ContactDao.h"
#include "ORM.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include "NetworkDataLoader.h"
#include "network.h"

ContactController::ContactController(Network *network, QObject* parent)
    : QObject(parent)
    , m_contactTreeModel(new ContactTreeModel(this))
    , m_networkDataLoader(network->networkDataLoader())
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

void ContactController::searchUser(const QString& keyword)
{
    if (keyword.trimmed().isEmpty()) {
        getAllContacts();
        return;
    }

    // 先尝试本地搜索
    QFutureWatcher<QVector<Contact>> *watcher = new QFutureWatcher<QVector<Contact>>(this);
    connect(watcher, &QFutureWatcher<QVector<Contact>>::finished, this, [this, watcher, keyword]() {
        QVector<Contact> contacts = watcher->result();
        if (!contacts.empty()) {
            emit searchUsered(contacts);
        } else {
            // 本地无结果，发起网络搜索
            QFutureWatcher<QPair<QJsonArray, QString>> *netWatcher = new QFutureWatcher<QPair<QJsonArray, QString>>(this);
            connect(netWatcher, &QFutureWatcher<QPair<QJsonArray, QString>>::finished, this, [this, netWatcher]() {
                auto result = netWatcher->result();
                const QJsonArray &users = result.first;
                const QString &error = result.second;

                if (!error.isEmpty()) {
                    qWarning() << "网络搜索失败:" << error;
                    emit searchUsered(QVector<Contact>()); // 或发射错误信号
                } else {
                    QVector<Contact> netContacts;
                    for (const QJsonValue &val : users) {
                        QJsonObject obj = val.toObject();
                        Contact contact;
                        contact.setuser_id(obj["user_id"].toVariant().toLongLong());
                        contact.setremark_name(obj["nickname"].toString());
                        contact.user.setaccount(obj["account"].toString());
                        contact.user.setavatar(obj["avatar"].toString());
                        contact.user.setuser_id(obj["user_id"].toString());
                        contact.user.setnickname(obj["nickname"].toString());
                        contact.user.setregion(obj["region"].toString());
                        netContacts.append(contact);
                    }
                    emit searchUsered(netContacts);
                }
                netWatcher->deleteLater();
            });

            // 在另一个线程中执行同步网络请求，避免阻塞 UI
            QFuture<QPair<QJsonArray, QString>> future = QtConcurrent::run([this, keyword]() {
                QJsonArray users;
                QString error;
                m_networkDataLoader->searchUsers(keyword, users, error);
                return qMakePair(users, error);
            });
            netWatcher->setFuture(future);
        }
        watcher->deleteLater();
    });

    QFuture<QVector<Contact>> future = QtConcurrent::run([keyword]() -> QVector<Contact> {
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

void ContactController::sendFriendRequest(QString &errorMessage,
                                          qint64 to_user_id,
                                          qint64 &outRequestId,
                                          const QString& message,
                                          const QString& remark,
                                          const QString& tags,
                                          const QString& source,
                                          const QString& description)
{
    m_networkDataLoader->sendFriendRequest(to_user_id,
                                           message,
                                           remark,
                                           tags,
                                           source,
                                           description,
                                           outRequestId,
                                           errorMessage);
}






