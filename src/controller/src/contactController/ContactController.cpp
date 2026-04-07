#include "ContactController.h"
#include "ContactDao.h"
#include "ORM.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include "NetworkDataLoader.h"
#include "network.h"
#include "WebSocketManager.h"

// 将服务端返回的好友 JSON 对象转换为 Contact 对象
static Contact parseContactFromJson(const QJsonObject &friendObj)
{
    Contact contact;
    // 注意：服务端返回的好友对象中，friend_id 才是联系人 user_id
    qint64 friendId = friendObj["friend_id"].toVariant().toLongLong();
    contact.setuser_id(friendId);
    contact.setremark_name(friendObj["remark"].toString());
    contact.setdescription(friendObj["description"].toString());
    contact.settags(friendObj["tags"].toString());
    contact.setphone_note(friendObj["phone_note"].toString());
    contact.setemail_note(friendObj["email_note"].toString());
    contact.setsource(friendObj["source"].toString());
    contact.setis_starred(friendObj["is_starred"].toInt());
    contact.setis_blocked(friendObj["is_blocked"].toInt());

    // 时间转换：服务端返回 "created_at": "yyyy-MM-dd HH:mm:ss"
    qint64 addTime = 0;
    QString createdAtStr = friendObj["created_at"].toString();
    if (!createdAtStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(createdAtStr, "yyyy-MM-dd HH:mm:ss");
        if (dt.isValid())
            addTime = dt.toSecsSinceEpoch();
        else
            qWarning() << "Invalid created_at format:" << createdAtStr;
    }
    contact.setadd_time(addTime);
    return contact;
}

// 将服务端返回的 user 子对象转换为 User 对象
static User parseUserFromJson(const QJsonObject &userObj, qint64 currentUserId = -1)
{
    User user;
    qint64 userId = userObj["user_id"].toVariant().toLongLong();
    user.setuser_id(userId);
    user.setaccount(userObj["account"].toString());
    user.setnickname(userObj["nickname"].toString());
    user.setavatar(userObj["avatar"].toString());
    user.setavatar_local_path("");   // 本地缓存路径后续再设置
    user.setprofile_cover(userObj["profile_cover"].toString());
    user.setgender(userObj["gender"].toInt());
    user.setregion(userObj["region"].toString());
    user.setsignature(userObj["signature"].toString());
    user.setis_current((userId == currentUserId) ? 1 : 0);

    return user;
}



ContactController::ContactController(Network *network, QObject* parent)
    : QObject(parent)
    , m_contactTreeModel(new ContactTreeModel(this))
    , m_networkDataLoader(network->networkDataLoader())
{
    connect(WebSocketManager::instance(), &WebSocketManager::friendRequestReceived,
            this, &ContactController::onNewFriendRequestReceived);
}

ContactController::~ContactController()
{
}

void ContactController::reloadContactsFromDatabase()
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

void ContactController::getAllContacts()
{
    QFutureWatcher<QPair<QJsonArray, QString>> *watcher = new QFutureWatcher<QPair<QJsonArray, QString>>(this);
    connect(watcher, &QFutureWatcher<QPair<QJsonArray, QString>>::finished, this, [this, watcher]() {
        auto result = watcher->result();
        const QJsonArray &friendsArray = result.first;
        const QString &error = result.second;

        if (!error.isEmpty()) {
            qWarning() << "从服务器拉取好友列表失败:" << error;
            emit contactsLoadFailed(error);
            watcher->deleteLater();
            return;
        }

        qint64 currentUserId = m_networkDataLoader->getCurrentLoginUserID();  // 需要实现此方法，见下文

        QFutureWatcher<bool> *dbWatcher = new QFutureWatcher<bool>(this);
        connect(dbWatcher, &QFutureWatcher<bool>::finished, this, [this, dbWatcher]() {
            if (dbWatcher->result()) {
                reloadContactsFromDatabase();  // 重新从数据库加载模型
            } else {
                qWarning() << "同步好友列表到本地数据库失败";
                emit contactsLoadFailed("数据库写入失败");
            }
            dbWatcher->deleteLater();
        });

        QFuture<bool> dbFuture = QtConcurrent::run([friendsArray, currentUserId]() -> bool {
            Orm orm;
            if (!orm.transaction()) return false;

            // 1. 收集服务器返回的好友 ID 集合
            QSet<qint64> serverFriendIds;
            for (const QJsonValue &val : friendsArray) {
                QJsonObject friendObj = val.toObject();
                QJsonObject userObj = friendObj["user"].toObject();
                qint64 friendId = userObj["user_id"].toVariant().toLongLong();
                if (friendId != -1)
                    serverFriendIds.insert(friendId);
            }

            // 2. 删除本地有但服务器没有的好友（从 contacts 表中删除）
            QList<Contact> localContacts = orm.findAll<Contact>();
            for (const Contact &localContact : localContacts) {
                if (!serverFriendIds.contains(localContact.user_idValue())) {
                    if (!orm.remove(localContact)) {
                        orm.rollback();
                        return false;
                    }
                }
            }

            // 3. 对服务器每个好友执行 upsert（更新或插入）
            for (const QJsonValue &val : friendsArray) {
                QJsonObject friendObj = val.toObject();
                QJsonObject userObj = friendObj["user"].toObject();
                qint64 friendId = userObj["user_id"].toVariant().toLongLong();
                if (friendId == -1) continue;

                // 3.1 处理 User 表（原逻辑不变）
                User friendUser = parseUserFromJson(userObj, currentUserId);
                auto existingUser = orm.findById<User>(friendId);
                if (existingUser.has_value()) {
                    if (!orm.update(friendUser)) {
                        orm.rollback();
                        return false;
                    }
                } else {
                    if (!orm.insert(friendUser)) {
                        orm.rollback();
                        return false;
                    }
                }

                // 3.2 处理 Contact 表：存在则更新，否则插入
                Contact contact = parseContactFromJson(friendObj);
                auto existingContact = orm.findById<Contact>(friendId);
                bool ok;
                if (existingContact.has_value())
                    ok = orm.update(contact);
                else
                    ok = orm.insert(contact);
                if (!ok) {
                    orm.rollback();
                    return false;
                }
            }

            return orm.commit();
        });

        dbWatcher->setFuture(dbFuture);
        watcher->deleteLater();
    });

    QFuture<QPair<QJsonArray, QString>> future = QtConcurrent::run([this]() {
        QJsonArray friendsArray;
        QString error;
        if (!m_networkDataLoader->loadFriends(friendsArray, error))
            return qMakePair(QJsonArray(), error);
        return qMakePair(friendsArray, error);
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
                        contact.user.setuser_id(obj["user_id"].toVariant().toLongLong());
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
    QFutureWatcher<QPair<QJsonObject, QString>> *watcher = new QFutureWatcher<QPair<QJsonObject, QString>>(this);
    connect(watcher, &QFutureWatcher<QPair<QJsonObject, QString>>::finished, this, [this, watcher, userId]() {
        auto result = watcher->result();
        const QJsonObject &friendData = result.first;
        const QString &error = result.second;

        if (!error.isEmpty()) {
            qWarning() << "从服务器获取好友详情失败:" << error;
            emit contactLoadFailed(userId, error);
            watcher->deleteLater();
            return;
        }

        qint64 currentUserId = m_networkDataLoader->getCurrentLoginUserID();
        QFutureWatcher<bool> *dbWatcher = new QFutureWatcher<bool>(this);
        connect(dbWatcher, &QFutureWatcher<bool>::finished, this, [this, dbWatcher, friendData, userId, currentUserId]() {
            if (dbWatcher->result()) {
                // 更新模型
                Contact contact = parseContactFromJson(friendData);
                m_contactTreeModel->addContact(contact);
                emit contactUpdated(userId);
            } else {
                qWarning() << "更新本地好友数据失败";
            }
            dbWatcher->deleteLater();
        });

        QFuture<bool> dbFuture = QtConcurrent::run([friendData, currentUserId]() -> bool {
            Orm orm;
            if (!orm.transaction()) return false;

            QJsonObject userObj = friendData["user"].toObject();
            qint64 friendId = userObj["user_id"].toVariant().toLongLong();
            if (friendId == 0) return false;

            // 更新 User
            User friendUser = parseUserFromJson(userObj, currentUserId);
            auto existingUser = orm.findById<User>(friendId);
            if (existingUser.has_value()) {
                if (!orm.update(friendUser)) {
                    orm.rollback();
                    return false;
                }
            } else {
                if (!orm.insert(friendUser)) {
                    orm.rollback();
                    return false;
                }
            }

            // 更新 Contact（存在则更新，不存在则插入）
            Contact contact = parseContactFromJson(friendData);
            auto existingContact = orm.findById<Contact>(friendId);
            bool ok;
            if (existingContact.has_value())
                ok = orm.update(contact);
            else
                ok = orm.insert(contact);
            if (!ok) {
                orm.rollback();
                return false;
            }

            return orm.commit();
        });
        dbWatcher->setFuture(dbFuture);
        watcher->deleteLater();
    });

    QFuture<QPair<QJsonObject, QString>> future = QtConcurrent::run([this, userId]() {
        QJsonObject friendData;
        QString error;
        if (!m_networkDataLoader->getFriend(userId, friendData, error))
            return qMakePair(QJsonObject(), error);
        return qMakePair(friendData, error);
    });
    watcher->setFuture(future);
}

void ContactController::updateContact(const Contact& contact)
{
    // 构建服务器允许的更新字段
    QJsonObject updateData;
    updateData["remark"] = contact.remark_nameValue();
    updateData["tags"] = contact.tagsValue();
    updateData["is_starred"] = contact.is_starredValue();
    updateData["is_blocked"] = contact.is_blockedValue();
    // 注意：description, phone_note, email_note, source 等服务端是否允许？根据 RestfulFriendCtrl 只允许这些，若需要可追加，但服务端代码只删除了 id, user_id, friend_id, created_at, updated_at, status，其他字段如 description 也会被更新，所以可以全部传。但为了安全，只传服务端明确允许的字段。

    QFutureWatcher<QPair<bool, QString>> *watcher = new QFutureWatcher<QPair<bool, QString>>(this);
    connect(watcher, &QFutureWatcher<QPair<bool, QString>>::finished, this, [this, watcher, contact]() {
        auto result = watcher->result();
        if (!result.first) {
            qWarning() << "更新服务器失败:" << result.second;
            emit contactUpdateFailed(contact.user_idValue(), result.second);
            watcher->deleteLater();
            return;
        }

        // 服务器成功，更新本地数据库
        QFutureWatcher<bool> *dbWatcher = new QFutureWatcher<bool>(this);
        connect(dbWatcher, &QFutureWatcher<bool>::finished, this, [this, dbWatcher, contact]() {
            if (dbWatcher->result()) {
                m_contactTreeModel->addContact(contact);
                emit contactUpdated(contact.user_idValue());
            } else {
                qWarning() << "本地更新失败，但服务器已更新，尝试重新拉取";
                getContact(contact.user_idValue());
            }
            dbWatcher->deleteLater();
        });

        QFuture<bool> dbFuture = QtConcurrent::run([contact]() -> bool {
            Orm orm;
            if (!orm.transaction()) return false;
            auto existing = orm.findById<Contact>(contact.user_idValue());
            bool ok;
            if (existing.has_value())
                ok = orm.update(contact);
            else
                ok = orm.insert(contact);
            if (ok) return orm.commit();
            else {
                orm.rollback();
                return false;
            }
        });
        dbWatcher->setFuture(dbFuture);
        watcher->deleteLater();
    });

    QFuture<QPair<bool, QString>> future = QtConcurrent::run([this, userId = contact.user_idValue(), updateData]() {
        QString error;
        bool ok = m_networkDataLoader->updateFriend(userId, updateData, error);
        return qMakePair(ok, error);
    });
    watcher->setFuture(future);
}

void ContactController::deleteContact(qint64 userId)
{
    QFutureWatcher<QPair<bool, QString>> *watcher = new QFutureWatcher<QPair<bool, QString>>(this);
    connect(watcher, &QFutureWatcher<QPair<bool, QString>>::finished, this, [this, watcher, userId]() {
        auto result = watcher->result();
        if (!result.first) {
            qWarning() << "删除服务器好友失败:" << result.second;
            emit contactDeleteFailed(userId, result.second);
            watcher->deleteLater();
            return;
        }

        // 服务器成功，删除本地记录
        QFutureWatcher<bool> *dbWatcher = new QFutureWatcher<bool>(this);
        connect(dbWatcher, &QFutureWatcher<bool>::finished, this, [this, dbWatcher, userId]() {
            if (dbWatcher->result()) {
                m_contactTreeModel->removeContact(userId);
                emit contactRemoved(userId);
            } else {
                qWarning() << "本地删除失败，但服务器已删除";
            }
            dbWatcher->deleteLater();
        });

        QFuture<bool> dbFuture = QtConcurrent::run([userId]() -> bool {
            Orm orm;
            auto contactOpt = orm.findById<Contact>(userId);
            if (!contactOpt.has_value())
                return true;   // 本地本来就不存在，视为成功
            return orm.remove(contactOpt.value());
        });
        dbWatcher->setFuture(dbFuture);
        watcher->deleteLater();
    });

    QFuture<QPair<bool, QString>> future = QtConcurrent::run([this, userId]() {
        QString error;
        bool ok = m_networkDataLoader->deleteFriend(userId, error);
        return qMakePair(ok, error);
    });
    watcher->setFuture(future);
}

void ContactController::setContactStarred(qint64 userId, bool starred)
{
    // 先从本地模型获取，若没有则从数据库获取
    Contact contact = getContactFromModel(userId);
    if (!contact.isValid()) {
        // 从数据库加载
        Orm orm;
        auto opt = orm.findById<Contact>(userId);
        if (!opt.has_value()) {
            qWarning() << "setContactStarred: contact not found in local, skip";
            return;
        }
        contact = opt.value();
    }
    contact.setis_starred(starred ? 1 : 0);
    updateContact(contact);
}

void ContactController::setContactBlocked(qint64 userId, bool blocked)
{
    Contact contact = getContactFromModel(userId);
    if (!contact.isValid()) {
        Orm orm;
        auto opt = orm.findById<Contact>(userId);
        if (!opt.has_value()) return;
        contact = opt.value();
    }
    contact.setis_blocked(blocked ? 1 : 0);
    updateContact(contact);
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



void ContactController::loadFriendRequests()
{
    // 步骤1：从网络拉取最新待处理申请
    QFutureWatcher<QList<FriendRequest>> *watcher = new QFutureWatcher<QList<FriendRequest>>(this);
    connect(watcher, &QFutureWatcher<QList<FriendRequest>>::finished, this, [this, watcher]() {
        QList<FriendRequest> requests = watcher->result();
        // 步骤2：同步到本地数据库并刷新模型
        syncFriendRequestsToLocalAndModel(requests);
        watcher->deleteLater();
    });

    // 在后台线程执行网络请求（避免阻塞 UI）
    QFuture<QList<FriendRequest>> future = QtConcurrent::run([this]() -> QList<FriendRequest> {
        QList<FriendRequest> requests;
        QString error;
        if (!m_networkDataLoader->getPendingFriendRequests(requests, error)) {
            qWarning() << "拉取好友申请失败:" << error;
        }
        return requests;
    });
    watcher->setFuture(future);
}

void ContactController::syncFriendRequestsToLocalAndModel(const QList<FriendRequest>& requests)
{
    // 在后台线程执行数据库操作，完成后在主线程更新模型
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, requests]() {
        if (watcher->result()) {
            QList<FriendRequest> allRequests;
            Orm orm;
            allRequests = orm.findWhere<FriendRequest>("1=1 ORDER BY created_at DESC");
            m_contactTreeModel->loadFriendRequests(allRequests);
            emit friendRequestsLoaded();
        } else {
            qWarning() << "同步好友申请到数据库失败";
        }
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([requests]() -> bool {
        Orm orm;
        if (!orm.transaction()) return false;

        // 1. 收集服务端返回的申请ID集合
        QSet<qint64> serverRequestIds;
        for (const FriendRequest& req : requests) {
            serverRequestIds.insert(req.idValue());
        }

        // 2. 删除本地存在但服务端不存在的好友申请
        QList<FriendRequest> localRequests = orm.findAll<FriendRequest>();
        for (const FriendRequest& localReq : localRequests) {
            qint64 reqId = localReq.idValue();
            if (!serverRequestIds.contains(reqId)) {
                if (!orm.remove(localReq)) {
                    orm.rollback();
                    return false;
                }
            }
        }

        // 3. 遍历服务端申请，存在更新，不存在插入
        for (const FriendRequest& req : requests) {
            qint64 reqId = req.idValue();
            if (reqId <= 0) {
                continue;
            }

            auto existReq = orm.findById<FriendRequest>(reqId);
            bool ok = false;

            if (existReq.has_value()) {
                ok = orm.update(req);
            } else {
                ok = orm.insert(req);
            }

            if (!ok) {
                orm.rollback();
                return false;
            }
        }

        return orm.commit();
    });
    watcher->setFuture(future);
}

void ContactController::onNewFriendRequestReceived(qint64 requestId, qint64 fromUserId, const QString& message)
{
    Q_UNUSED(requestId);
    Q_UNUSED(fromUserId);
    Q_UNUSED(message);
    qDebug() << "收到新好友申请推送，重新拉取全量数据";
    loadFriendRequests();
}

void ContactController::processFriendRequest(
    qint64 requestId,
    bool agree,
    std::function<void(bool)> callback,
    const QString& remark,
    const QString& tags,
    const QString& description,
    const QString& source,
    int isStarred)
{
    // 构造 meta JSON 对象
    QJsonObject meta;
    if (!remark.isEmpty())   meta["remark"] = remark;
    if (!description.isEmpty()) meta["description"] = description;
    if (!source.isEmpty())   meta["source"] = source;
    if (isStarred != 0)      meta["is_starred"] = isStarred;
    // 标签字符串 → JSON数组
    QJsonArray tagArray;
    if (!tags.isEmpty()) {
        QStringList tagList = tags.split(",", Qt::SkipEmptyParts);
        for (const QString& tag : std::as_const(tagList)) {
            tagArray.append(tag.trimmed());
        }
    }
    meta["tags"] = tagArray;

    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, requestId, agree, callback]() {
        bool success = watcher->result();
        if (success) {
            loadFriendRequests();  // 刷新模型
            emit friendRequestProcessed(requestId, true);
        } else {
            emit friendRequestProcessed(requestId, false);
        }
        if (callback) callback(success);
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([this, requestId, agree, meta]() -> bool {
        QString error;
        return m_networkDataLoader->processFriendRequest(requestId, agree, error, meta);
    });
    watcher->setFuture(future);
}




