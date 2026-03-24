#pragma once

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QMutex>
#include <QVariantList>
#include <optional>
#include "DbConnectionManager.h"
#include "ORM_Macros.h"  // 假设宏定义在此

class Orm {
public:
    Orm() {
        QSharedPointer<QSqlDatabase> threadDb = DbConnectionManager::connectionForCurrentThread();
        if (threadDb.isNull() || !threadDb->isOpen()) {
            qCritical() << "Orm: Failed to get valid database connection for current thread!";
            m_db = QSqlDatabase();
            return;
        }
        m_db = *threadDb;
    }

    // 获取最后一次操作错误
    QSqlError lastError() const {
        QMutexLocker locker(&m_mutex);
        return m_lastError;
    }

    // 根据主键查找单条记录
    template<typename T>
    std::optional<T> findById(qint64 id) {
        QMutexLocker locker(&m_mutex);
        QString sql = QString("SELECT %1 FROM %2 WHERE %3 = ?")
                          .arg(T::fields().join(','))
                          .arg(T::tableName())
                          .arg(T::primaryKey());
        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            m_lastError = query.lastError();
            return std::nullopt;
        }
        query.bindValue(0, id);
        if (!query.exec()) {
            m_lastError = query.lastError();
            return std::nullopt;
        }
        if (query.next()) {
            m_lastError = QSqlError();  // 清除错误
            return T::fromSqlQuery(query);
        }
        m_lastError = QSqlError();  // 未找到记录，无错误
        return std::nullopt;
    }

    // 查询所有记录
    template<typename T>
    QList<T> findAll() {
        return findWhere<T>(QString());
    }

    // 条件查询 + 排序 + 分页
    template<typename T>
    QList<T> findWhere(const QString& whereClause = QString(),
                       const QVariantList& params = {},
                       const QString& orderBy = QString(),
                       int limit = -1,
                       int offset = -1) {
        QMutexLocker locker(&m_mutex);
        QString sql = QString("SELECT %1 FROM %2")
                          .arg(T::fields().join(','))
                          .arg(T::tableName());
        if (!whereClause.isEmpty())
            sql += " WHERE " + whereClause;
        if (!orderBy.isEmpty())
            sql += " ORDER BY " + orderBy;
        if (limit >= 0)
            sql += QString(" LIMIT %1").arg(limit);
        if (offset >= 0)
            sql += QString(" OFFSET %1").arg(offset);

        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            m_lastError = query.lastError();
            return {};
        }
        for (int i = 0; i < params.size(); ++i)
            query.bindValue(i, params[i]);
        if (!query.exec()) {
            m_lastError = query.lastError();
            return {};
        }

        QList<T> result;
        while (query.next())
            result << T::fromSqlQuery(query);
        m_lastError = QSqlError();  // 清除错误
        return result;
    }

    // 插入记录：只插入已设置的字段（非空），未设置字段跳过，使用数据库默认值（如 NULL）
    template<typename T>
    bool insert(T& obj) {
        QMutexLocker locker(&m_mutex);
        QStringList flds = T::fields();
        QString pk = T::primaryKey();
        bool autoPk = obj.getField(pk).isNull();  // 主键未设置，需要自增

        QStringList insertFields;
        QVariantList values;
        for (const QString& f : flds) {
            if (autoPk && f == pk) continue;
            QVariant val = obj.getField(f);
            if (val.isNull()) {
                // 字段未设置，跳过插入，让数据库使用默认值
                continue;
            }
            insertFields << f;
            values << val;
        }

        if (insertFields.isEmpty()) {
            // 没有要插入的字段，视为无效操作
            m_lastError = QSqlError("No fields to insert", "", QSqlError::StatementError);
            return false;
        }

        QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
                          .arg(T::tableName())
                          .arg(insertFields.join(','))
                          .arg(QString("?,").repeated(values.size()).chopped(1));

        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            m_lastError = query.lastError();
            return false;
        }
        if (!execWithBind(query, values)) {
            m_lastError = query.lastError();
            return false;
        }
        if (!query.exec()) {
            m_lastError = query.lastError();
            return false;
        }

        if (autoPk) {
            // 自增主键写回对象
            obj.setField(pk, query.lastInsertId());
        }

        m_lastError = QSqlError();  // 清除错误
        return true;
    }

    // 更新记录：只更新已设置的字段（非空），主键必须存在
    template<typename T>
    bool update(const T& obj) {
        QMutexLocker locker(&m_mutex);
        QStringList flds = T::fields();
        QString pk = T::primaryKey();
        QVariant pkVal = obj.getField(pk);
        if (pkVal.isNull()) {
            m_lastError = QSqlError("Primary key is null", "", QSqlError::StatementError);
            return false;
        }

        QStringList setClauses;
        QVariantList values;
        for (const QString& f : flds) {
            if (f == pk) continue;
            QVariant val = obj.getField(f);
            if (val.isNull()) {
                // 字段未设置，跳过更新（保留原值）
                continue;
            }
            setClauses << f + " = ?";
            values << val;
        }

        if (setClauses.isEmpty()) {
            // 没有要更新的字段，视为无操作（成功）
            return true;
        }

        QString sql = QString("UPDATE %1 SET %2 WHERE %3 = ?")
                          .arg(T::tableName())
                          .arg(setClauses.join(','))
                          .arg(pk);
        values << pkVal;  // WHERE 条件

        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            m_lastError = query.lastError();
            return false;
        }
        if (!execWithBind(query, values)) {
            m_lastError = query.lastError();
            return false;
        }
        if (!query.exec()) {
            m_lastError = query.lastError();
            return false;
        }

        m_lastError = QSqlError();  // 清除错误
        return true;
    }

    // 删除记录
    template<typename T>
    bool remove(const T& obj) {
        QMutexLocker locker(&m_mutex);
        QString pk = T::primaryKey();
        QVariant pkVal = obj.getField(pk);
        if (pkVal.isNull()) {
            m_lastError = QSqlError("Primary key is null", "", QSqlError::StatementError);
            return false;
        }

        QString sql = QString("DELETE FROM %1 WHERE %2 = ?")
                          .arg(T::tableName()).arg(pk);
        QSqlQuery query(m_db);
        if (!query.prepare(sql)) {
            m_lastError = query.lastError();
            return false;
        }
        query.bindValue(0, pkVal);
        if (!query.exec()) {
            m_lastError = query.lastError();
            return false;
        }

        m_lastError = QSqlError();  // 清除错误
        return true;
    }

    // 事务操作
    bool transaction() {
        QMutexLocker locker(&m_mutex);
        bool ok = m_db.transaction();
        if (!ok) m_lastError = m_db.lastError();
        else m_lastError = QSqlError();
        return ok;
    }

    bool commit() {
        QMutexLocker locker(&m_mutex);
        bool ok = m_db.commit();
        if (!ok) m_lastError = m_db.lastError();
        else m_lastError = QSqlError();
        return ok;
    }

    bool rollback() {
        QMutexLocker locker(&m_mutex);
        bool ok = m_db.rollback();
        if (!ok) m_lastError = m_db.lastError();
        else m_lastError = QSqlError();
        return ok;
    }

    // 获取数据库连接（供外部原生 SQL 使用）
    const QSqlDatabase& database() const { return m_db; }

private:
    QSqlDatabase m_db;
    mutable QMutex m_mutex;
    QSqlError m_lastError;

    bool execWithBind(QSqlQuery& query, const QVariantList& values) {
        for (int i = 0; i < values.size(); ++i) {
            query.bindValue(i, values[i]);
            if (query.lastError().isValid()) {
                m_lastError = query.lastError();
                return false;
            }
        }
        return true;
    }
};
