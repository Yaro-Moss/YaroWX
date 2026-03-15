#include "DatabaseInitializer.h"
#include "ConfigManager.h"
#include "DatabaseSchema.h"
#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <qthread.h>

std::atomic<bool> DatabaseInitializer::s_initialized{false};
QMutex DatabaseInitializer::s_initMutex;

DatabaseInitializer::DatabaseInitializer(QObject* parent)
    : QObject(parent)
{
    m_dbPath = databasePath();
    // 如果外部已经初始化过，标记也要同步
    if (!s_initialized.load()) {
        QMutexLocker lock(&s_initMutex);
        if (!s_initialized.load()) {
            if (databaseFileExists()) {
                s_initialized.store(true);
                qDebug() << "Database already exists and is valid, marked as initialized";
            }
        }
    }
}

DatabaseInitializer::~DatabaseInitializer()
{
    // 先销毁所有可能的查询对象，再关闭连接
    QSqlDatabase db;
    if (QSqlDatabase::contains("main")) {
        db = QSqlDatabase::database("main");
        if (db.isOpen()) {
            db.close();
        }
        // 手动销毁查询对象（避免连接被占用）
        QSqlQuery().finish();
    }
    QSqlDatabase::removeDatabase("main");
}

bool DatabaseInitializer::ensureInitialized()
{
    if (s_initialized.load()) return true;

    // 确保目录存在
    QDir dir(QFileInfo(m_dbPath).absolutePath());
    if (!dir.mkpath(".")) {
        qCritical() << "Failed to create directory for DB:" << dir.absolutePath();
        return false;
    }

    // 先清理旧连接（确保查询对象销毁）
    if (QSqlDatabase::contains("main")) {
        QSqlDatabase oldDb = QSqlDatabase::database("main");
        if (oldDb.isOpen()) {
            oldDb.close();
        }
        // 手动销毁所有可能的查询对象
        QSqlQuery().finish();
        QSqlDatabase::removeDatabase("main");
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "main");
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        qCritical() << "Failed to open main DB:" << db.lastError().text();
        // 关闭连接并删除新建的损坏文件
        removeDatabaseFile();
        return false;
    }

    applyPragmas(db);

    // 声明查询对象在栈上，确保作用域结束后销毁
    QSqlQuery q(db);
    bool ok = createTables(db);

    // 显式结束查询、关闭连接
    q.finish();
    db.close();
    QSqlDatabase::removeDatabase("main");

    if (!ok) {
        qCritical() << "Database initialization failed";
        removeDatabaseFile();
        return false;
    }

    s_initialized.store(true);
    qDebug() << "Database initialized successfully at" << m_dbPath;
    return true;
}

QString DatabaseInitializer::databasePath()
{
    ConfigManager* configManager = ConfigManager::instance();
    QDir dir(configManager->dataSavePath());
    dir.mkpath(".");
    return dir.absoluteFilePath("YaroWX.db");
}

bool DatabaseInitializer::databaseFileExists() const
{
    QFileInfo fi(m_dbPath);
    return fi.exists() && fi.size() > 0;
}

void DatabaseInitializer::applyPragmas(QSqlDatabase &db)
{
    const QStringList pragmas = {
        "PRAGMA foreign_keys = ON",
        "PRAGMA journal_mode = WAL",
        "PRAGMA synchronous = NORMAL",
        "PRAGMA cache_size = -64000",
        "PRAGMA temp_store = MEMORY"
    };
    for (const QString &p : pragmas) {
        QSqlQuery q(db);
        if (!q.exec(p))
            qWarning() << "Failed to set pragma" << p << q.lastError().text();
    }
}

bool DatabaseInitializer::createTables(QSqlDatabase &db)
{
    // 开始事务
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction for creating tables:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);

    // 1. 先创建所有表
    const QStringList tables = {
        DatabaseSchema::getCreateTableUser(),
        DatabaseSchema::getCreateTableContacts(),
        DatabaseSchema::getCreateTableGroups(),
        DatabaseSchema::getCreateTableGroupMembers(),
        DatabaseSchema::getCreateTableConversations(),
        DatabaseSchema::getCreateTableMessages(),
        DatabaseSchema::getCreateTableMediaCache(),
        DatabaseSchema::getCreateTableLocalMoment(),
        DatabaseSchema::getCreateTableLocalMomentInteract()
    };

    for (const QString &sql : tables) {
        if (!q.exec(sql)) {
            db.rollback();
            qCritical() << "Create table failed:" << q.lastError().text() << "SQL:" << sql;
            return false;
        }
    }

    // 2. 提交表创建事务
    if (!db.commit()) {
        qCritical() << "Commit failed after creating tables:" << db.lastError().text();
        return false;
    }

    // 3. 开始新的事务创建索引和触发器
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction for indexes and triggers:" << db.lastError().text();
        return false;
    }

    // 创建索引
    const QStringList indexes = DatabaseSchema::getCreateIndexes().split(';', Qt::SkipEmptyParts);
    for (const QString& sql : indexes) {
        const QString trimmedSql = sql.trimmed();
        if (!trimmedSql.isEmpty()) {
            if (!q.exec(trimmedSql)) {
                qWarning() << "Create index failed:" << q.lastError().text() << "SQL:" << trimmedSql;
                // 索引创建失败不中断，继续执行
            }
        }
    }

    // 创建触发器
    const QStringList triggers = DatabaseSchema::getCreateTriggers();
    for (const QString& sql : triggers) {
        const QString trimmedSql = sql.trimmed();
        if (!trimmedSql.isEmpty()) {
            if (!q.exec(trimmedSql)) {
                qWarning() << "Create trigger failed:" << q.lastError().text() << "SQL:" << trimmedSql;
                // 触发器创建失败不中断，继续执行
            }
        }
    }

    // 提交索引和触发器事务
    if (!db.commit()) {
        qCritical() << "Commit failed for indexes and triggers:" << db.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseInitializer::removeDatabaseFile()
{
    if (m_dbPath.isEmpty()) {
        return false;
    }

    // 确保连接完全关闭且查询对象销毁
    QSqlDatabase db;
    if (QSqlDatabase::contains("main")) {
        db = QSqlDatabase::database("main");
        if (db.isOpen()) {
            db.close();
        }
        QSqlQuery().finish();
        QSqlDatabase::removeDatabase("main");
    }

    // 延迟100ms确保文件句柄释放（Windows下文件删除可能卡顿）
    QThread::msleep(100);

    QFile dbFile(m_dbPath);
    bool removed = false;
    if (dbFile.exists()) {
        removed = dbFile.remove();
        if (removed) {
            qDebug() << "Removed corrupted database file:" << m_dbPath;
            // 同时删除WAL和SHM文件
            QFile::remove(m_dbPath + "-wal");
            QFile::remove(m_dbPath + "-shm");
        } else {
            qWarning() << "Failed to remove database file:" << m_dbPath;
        }
    }

    return removed;
}
