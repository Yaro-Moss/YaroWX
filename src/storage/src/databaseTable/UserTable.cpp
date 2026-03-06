#include "UserTable.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QDateTime>
#include <QSqlRecord>
#include "DbConnectionManager.h"
#include <QDebug>
#include <stdexcept>

UserTable::UserTable(QObject *parent)
    : QObject(parent)
{
}

UserTable::~UserTable()
{
    // 不需要手动关闭连接，智能指针会自动管理
}

bool UserTable::init()
{
    m_database = DbConnectionManager::connectionForCurrentThread();
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        QString errorText = m_database ? m_database->lastError().text() : "Failed to get database connection";
        emit dbError(-1, QString("Open DB failed: %1").arg(errorText));
        return false;
    }
    return true;
}

bool UserTable::ensureDbOpen(int reqId)
{
    if (!m_database || !m_database->isValid() || !m_database->isOpen()) {
        if (reqId >= 0) emit dbError(reqId, "Database is not valid or not open");
        return false;
    }
    return true;
}

// ----- 当前用户管理 -----
bool UserTable::saveCurrentUser(int reqId, const User &user)
{
    // 1. 先确保数据库已打开，未打开则直接返回失败
    if (!ensureDbOpen(reqId)) {
        emit currentUserSaved(reqId, false, "Database not open");
        return false;
    }

    // 2. 开启事务（所有操作要么全成功，要么全回滚，避免数据不一致）
    if (!m_database->transaction()) {
        emit currentUserSaved(reqId, false, "Begin transaction failed: " + m_database->lastError().text());
        return false;
    }

    try {
        // ---这里有bug，暂时只是测试，还不能改
        QSqlQuery deleteOldCurrentQuery(*m_database);
        const QString deleteSql = "DELETE FROM users WHERE is_current = 1";
        if (!deleteOldCurrentQuery.exec(deleteSql)) {
            throw std::runtime_error(
                QString("Delete old current user failed: %1").arg(deleteOldCurrentQuery.lastError().text()).toStdString()
                );
        }

        QSqlQuery insertNewUserQuery(*m_database);
        const QString insertSql = R"(
            INSERT OR REPLACE INTO users
            (user_id, account, nickname, avatar, avatar_local_path, profile_cover, gender, region, signature, is_current)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";
        insertNewUserQuery.prepare(insertSql);
        // 绑定参数（与 User 结构体字段一一对应，避免SQL注入）
        insertNewUserQuery.addBindValue(user.userId);
        insertNewUserQuery.addBindValue(user.account);
        insertNewUserQuery.addBindValue(user.nickname);
        insertNewUserQuery.addBindValue(user.avatar);
        insertNewUserQuery.addBindValue(user.avatarLocalPath);
        insertNewUserQuery.addBindValue(user.profile_cover);
        insertNewUserQuery.addBindValue(user.gender);
        insertNewUserQuery.addBindValue(user.region);
        insertNewUserQuery.addBindValue(user.signature);
        insertNewUserQuery.addBindValue(1); // 新用户标记为“当前用户”

        if (!insertNewUserQuery.exec()) {
            throw std::runtime_error(
                QString("Insert new current user failed: %1").arg(insertNewUserQuery.lastError().text()).toStdString()
                );
        }

        // -------------------------- 提交事务：所有操作成功后确认 --------------------------
        if (!m_database->commit()) {
            throw std::runtime_error(
                QString("Commit transaction failed: %1").arg(m_database->lastError().text()).toStdString()
                );
        }

        emit currentUserSaved(reqId, true, "Save current user success");
        qInfo() << "[UserTable] Save current user success (user_id:" << user.userId << ")";
        return true;

    } catch (const std::exception &e) {
        // 异常处理：回滚事务，通知失败并打印错误日志
        m_database->rollback();
        const QString errorReason = QString::fromUtf8(e.what());
        qCritical() << "[UserTable] Save current user failed (reqId:" << reqId << "):" << errorReason;
        emit currentUserSaved(reqId, false, errorReason);
        return false;
    }
}

void UserTable::getCurrentUser(int reqId)
{
    if (!ensureDbOpen(reqId)) { emit currentUserLoaded(reqId, User()); return; }

    QSqlQuery q(*m_database);
    q.prepare("SELECT * FROM users WHERE is_current = 1 LIMIT 1");
    if (!q.exec() || !q.next()) {
        emit currentUserLoaded(reqId, User());
        return;
    }
    emit currentUserLoaded(reqId, User(q));
}

void UserTable::updateCurrentUser(int reqId, const User &user)
{
    saveCurrentUser(reqId, user);
}

void UserTable::clearCurrentUser(int reqId)
{
    if (!ensureDbOpen(reqId)) { emit currentUserSaved(reqId, false, "Database not open"); return; }

    QSqlQuery q(*m_database);
    if (!q.exec("UPDATE users SET is_current = 0 WHERE is_current = 1")) {
        emit currentUserSaved(reqId, false, q.lastError().text());
        return;
    }
    emit currentUserSaved(reqId, true, QString());
}

// ----- 通用的增删改查（CRUD） -----
void UserTable::saveUser(int reqId, const User &user)
{
    if (!ensureDbOpen(reqId)) { emit userSaved(reqId, false, "Database not open"); return; }

    QSqlQuery q(*m_database);
    q.prepare(R"(
        INSERT OR REPLACE INTO users
        (user_id, account, nickname, avatar, avatar_local_path, profile_cover, gender, region, signature, is_current)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(user.userId);
    q.addBindValue(user.account);
    q.addBindValue(user.nickname);
    q.addBindValue(user.avatar);
    q.addBindValue(user.avatarLocalPath);
    q.addBindValue(user.profile_cover);
    q.addBindValue(user.gender);
    q.addBindValue(user.region);
    q.addBindValue(user.signature);
    q.addBindValue(user.isCurrent ? 1 : 0);

    if (!q.exec()) {
        emit userSaved(reqId, false, q.lastError().text());
        return;
    }
    emit userSaved(reqId, true, QString());
}

void UserTable::updateUser(int reqId, const User &user)
{
    // 暂时用不到，懒得写了，先用saveUser哈哈
    saveUser(reqId, user);
}

void UserTable::deleteUser(int reqId, qint64 userId)
{
    if (!ensureDbOpen(reqId)) { emit userDeleted(reqId, false, "Database not open"); return; }

    QSqlQuery q(*m_database);
    q.prepare("DELETE FROM users WHERE user_id = ?");
    q.addBindValue(userId);

    if (!q.exec()) {
        emit userDeleted(reqId, false, q.lastError().text());
        return;
    }
    emit userDeleted(reqId, q.numRowsAffected() > 0, QString());
}

void UserTable::getUser(int reqId, qint64 userId)
{
    if (!ensureDbOpen(reqId)) { emit userLoaded(reqId, User()); return; }

    QSqlQuery q(*m_database);
    q.prepare("SELECT * FROM users WHERE user_id = ?");
    q.addBindValue(userId);

    if (!q.exec() || !q.next()) {
        emit userLoaded(reqId, User());
        return;
    }
    emit userLoaded(reqId, User(q));
}

// ----- 辅助查询 -----
void UserTable::getUserByAccount(int reqId, const QString &account)
{
    if (!ensureDbOpen(reqId)) { emit userByAccountLoaded(reqId, User()); return; }

    QSqlQuery q(*m_database);
    q.prepare("SELECT * FROM users WHERE account = ?");
    q.addBindValue(account);
    if (!q.exec() || !q.next()) {
        emit userByAccountLoaded(reqId, User());
        return;
    }
    emit userByAccountLoaded(reqId, User(q));
}

void UserTable::getAllUsers(int reqId)
{
    QList<User> users;
    if (!ensureDbOpen(reqId)) { emit allUsersLoaded(reqId, users); return; }

    QSqlQuery q(*m_database);
    q.prepare("SELECT * FROM users ORDER BY user_id");
    if (!q.exec()) {
        emit dbError(reqId, q.lastError().text());
        emit allUsersLoaded(reqId, users);
        return;
    }
    while (q.next()) users.append(User(q));
    emit allUsersLoaded(reqId, users);
}

