#include "UserController.h"
#include "ORM.h"
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>

UserController::UserController(QObject* parent)
    : QObject(parent)
{
}

UserController::~UserController()
{
}

void UserController::addUser(const User& user)
{
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, user]() {
                bool success = watcher->result();
                if (success) {
                    emit userSaved(user.user_idValue(), true);
                    // 如果添加的用户正好是当前登录用户，更新内存中的用户信息
                    if (m_currentLoginUser.isValid() &&
                        m_currentLoginUser.user_id() == user.user_id()) {
                        m_currentLoginUser.user = user;
                        emit currentUserUpdated();
                    }
                } else {
                    emit userSaved(user.user_idValue(), false, "Database error");
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([user]() -> bool {
        Orm orm;
        User copy = user;
        return orm.insert(copy);
    });
    watcher->setFuture(future);
}

void UserController::updateUser(const User& user)
{
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, user]() {
                bool success = watcher->result();
                if (success) {
                    emit userUpdated(user.user_idValue());
                    // 如果是当前登录用户，更新内存中的信息
                    if (m_currentLoginUser.isValid() &&
                        m_currentLoginUser.user_id() == user.user_id()) {
                        m_currentLoginUser.user = user;
                        emit currentUserUpdated();
                    }
                } else {
                    qWarning() << "Update user failed:" << user.user_id();
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([user]() -> bool {
        Orm orm;
        return orm.update(user);
    });
    watcher->setFuture(future);
}

void UserController::deleteUser(qint64 userId)
{
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, userId]() {
                bool success = watcher->result();
                if (success) {
                    emit userDeleted(userId);
                    // 如果删除的是当前登录用户，清空当前用户
                    if (m_currentLoginUser.isValid() &&
                        m_currentLoginUser.user_id() == userId) {
                        m_currentLoginUser = Contact();
                        emit currentUserUpdated();
                    }
                } else {
                    qWarning() << "Delete user failed:" << userId;
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([userId]() -> bool {
        Orm orm;
        User dummy;
        dummy.setuser_id(userId);
        return orm.remove(dummy);
    });
    watcher->setFuture(future);
}

void UserController::getUser(qint64 userId)
{
    QFutureWatcher<User>* watcher = new QFutureWatcher<User>(this);
    connect(watcher, &QFutureWatcher<User>::finished, this,
            [this, watcher, userId]() {
                User user = watcher->result();
                if (user.isValid()) {
                    emit userLoaded(userId, user);
                } else {
                    qWarning() << "User not found:" << userId;
                }
                watcher->deleteLater();
            });

    QFuture<User> future = QtConcurrent::run([userId]() -> User {
        Orm orm;
        auto userOpt = orm.findById<User>(userId);
        if (userOpt) {
            return *userOpt;
        }
        return User();
    });
    watcher->setFuture(future);
}

void UserController::getAllUsers()
{
    QFutureWatcher<QList<User>>* watcher = new QFutureWatcher<QList<User>>(this);
    connect(watcher, &QFutureWatcher<QList<User>>::finished, this,
            [this, watcher]() {
                QList<User> users = watcher->result();
                emit allUsersLoaded(users);
                watcher->deleteLater();
            });

    QFuture<QList<User>> future = QtConcurrent::run([]() -> QList<User> {
        Orm orm;
        return orm.findAll<User>();
    });
    watcher->setFuture(future);
}

void UserController::updateNickname(const QString& nickname)
{
    if (!m_currentLoginUser.isValid()) {
        qWarning() << "No current user set";
        return;
    }
    qint64 userId = m_currentLoginUser.user_idValue();

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, userId, nickname]() {
                bool success = watcher->result();
                if (success) {
                    m_currentLoginUser.user.setnickname(nickname);
                    emit currentUserUpdated();
                    emit userUpdated(userId);
                } else {
                    qWarning() << "Update nickname failed for user:" << userId;
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([userId, nickname]() -> bool {
        Orm orm;
        auto userOpt = orm.findById<User>(userId);
        if (!userOpt) return false;
        User user = *userOpt;
        user.setnickname(nickname);
        return orm.update(user);
    });
    watcher->setFuture(future);
}

void UserController::updateAvatar(const QString& avatarUrl, const QString& localPath)
{
    if (!m_currentLoginUser.isValid()) {
        qWarning() << "No current user set";
        return;
    }
    qint64 userId = m_currentLoginUser.user_idValue();

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, userId, avatarUrl, localPath]() {
                bool success = watcher->result();
                if (success) {
                    m_currentLoginUser.user.setavatar(avatarUrl);
                    m_currentLoginUser.user.setavatar_local_path(localPath);
                    emit currentUserUpdated();
                    emit userUpdated(userId);
                } else {
                    qWarning() << "Update avatar failed for user:" << userId;
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([userId, avatarUrl, localPath]() -> bool {
        Orm orm;
        auto userOpt = orm.findById<User>(userId);
        if (!userOpt) return false;
        User user = *userOpt;
        user.setavatar(avatarUrl);
        user.setavatar_local_path(localPath);
        return orm.update(user);
    });
    watcher->setFuture(future);
}

void UserController::updateSignature(const QString& signature)
{
    if (!m_currentLoginUser.isValid()) {
        qWarning() << "No current user set";
        return;
    }
    qint64 userId = m_currentLoginUser.user_idValue();

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, userId, signature]() {
                bool success = watcher->result();
                if (success) {
                    m_currentLoginUser.user.setsignature(signature);
                    emit currentUserUpdated();
                    emit userUpdated(userId);
                } else {
                    qWarning() << "Update signature failed for user:" << userId;
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([userId, signature]() -> bool {
        Orm orm;
        auto userOpt = orm.findById<User>(userId);
        if (!userOpt) return false;
        User user = *userOpt;
        user.setsignature(signature);
        return orm.update(user);
    });
    watcher->setFuture(future);
}

void UserController::updateGender(int gender)
{
    if (!m_currentLoginUser.isValid()) {
        qWarning() << "No current user set";
        return;
    }
    qint64 userId = m_currentLoginUser.user_idValue();

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, userId, gender]() {
                bool success = watcher->result();
                if (success) {
                    m_currentLoginUser.user.setgender(gender);
                    emit currentUserUpdated();
                    emit userUpdated(userId);
                } else {
                    qWarning() << "Update gender failed for user:" << userId;
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([userId, gender]() -> bool {
        Orm orm;
        auto userOpt = orm.findById<User>(userId);
        if (!userOpt) return false;
        User user = *userOpt;
        user.setgender(gender);
        return orm.update(user);
    });
    watcher->setFuture(future);
}

void UserController::updateRegion(const QString& region)
{
    if (!m_currentLoginUser.isValid()) {
        qWarning() << "No current user set";
        return;
    }
    qint64 userId = m_currentLoginUser.user_idValue();

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, userId, region]() {
                bool success = watcher->result();
                if (success) {
                    m_currentLoginUser.user.setregion(region);
                    emit currentUserUpdated();
                    emit userUpdated(userId);
                } else {
                    qWarning() << "Update region failed for user:" << userId;
                }
                watcher->deleteLater();
            });

    QFuture<bool> future = QtConcurrent::run([userId, region]() -> bool {
        Orm orm;
        auto userOpt = orm.findById<User>(userId);
        if (!userOpt) return false;
        User user = *userOpt;
        user.setregion(region);
        return orm.update(user);
    });
    watcher->setFuture(future);
}
