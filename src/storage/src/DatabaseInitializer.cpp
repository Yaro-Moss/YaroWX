#include "DatabaseInitializer.h"
#include "ConfigManager.h"
#include "DatabaseSchema.h"
#include "DbConnectionManager.h"  // 引入连接管理器
#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QThread>

std::atomic<bool> DatabaseInitializer::s_initialized{false};
QMutex DatabaseInitializer::s_initMutex;
QMutex DatabaseInitializer::s_fileMutex;

DatabaseInitializer::DatabaseInitializer(QObject* parent)
    : QObject(parent)
{
    m_dbPath = databasePath();
    // 如果外部已经初始化过，同步标记
    if (!s_initialized.load()) {
        QMutexLocker lock(&s_initMutex);
        if (!s_initialized.load() && databaseFileExists()) {
            s_initialized.store(true);
            qDebug() << "Database already exists and is valid, marked as initialized";
        }
    }
}

// 析构函数：无需处理main连接，由DbConnectionManager自动管理
DatabaseInitializer::~DatabaseInitializer() = default;

bool DatabaseInitializer::ensureInitialized()
{
    // 双重检查锁：保证初始化只执行一次
    if (s_initialized.load()) return true;

    QMutexLocker lock(&s_initMutex);
    if (s_initialized.load()) return true;

    // 1. 确保数据库目录存在
    QDir dir(QFileInfo(m_dbPath).absolutePath());
    if (!dir.mkpath(".")) {
        qCritical() << "Failed to create directory for DB:" << dir.absolutePath();
        return false;
    }

    // 2. 获取当前线程的数据库连接（完全依赖DbConnectionManager）
    auto dbPtr = DbConnectionManager::connectionForCurrentThread();
    if (!dbPtr) {
        qCritical() << "Failed to get thread DB connection for initialization";
        removeDatabaseFile();
        return false;
    }
    QSqlDatabase& db = *dbPtr;

    // 3. 应用SQLite优化参数
    applyPragmas(db);

    // 4. 创建数据表、索引、触发器
    QSqlQuery q(db);
    bool initOk = createTables(db);
    q.finish();  // 显式结束查询，释放资源

    // 5. 处理初始化失败
    if (!initOk) {
        qCritical() << "Database initialization failed";
        removeDatabaseFile();
        return false;
    }

    // 6. 标记初始化完成
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
    return fi.exists() && fi.size() > 100;
}

void DatabaseInitializer::applyPragmas(QSqlDatabase &db)
{
    const QStringList pragmas = {
        "PRAGMA foreign_keys = ON",        // 开启外键约束
        "PRAGMA journal_mode = WAL",       // 写前日志模式（提升并发性能）
        "PRAGMA synchronous = NORMAL",     // 平衡性能和安全性
        "PRAGMA cache_size = -64000",      // 64MB缓存（负数表示页数，1页=1KB）
        "PRAGMA temp_store = MEMORY"       // 临时表存内存（提升速度）
    };

    QSqlQuery q(db);
    for (const QString &p : pragmas) {
        if (!q.exec(p)) {
            qWarning() << "Failed to set pragma" << p << "Error:" << q.lastError().text();
        }
    }
}

bool DatabaseInitializer::createTables(QSqlDatabase &db)
{
    // ========== 第一步：创建数据表（事务保护） ==========
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction for tables:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    const QStringList tableSqls = {
        DatabaseSchema::getCreateTableUser(),
        DatabaseSchema::getCreateTableContacts(),
        DatabaseSchema::getCreateTableGroups(),
        DatabaseSchema::getCreateTableGroupMembers(),
        DatabaseSchema::getCreateTableConversations(),
        DatabaseSchema::getCreateTableMessages(),
        DatabaseSchema::getCreateTableMediaCache(),
        DatabaseSchema::getCreateTableLocalMoment(),
        DatabaseSchema::getCreateTableLocalMomentInteract(),
        DatabaseSchema::getCreateTableFriendRequests()
    };

    for (const QString &sql : tableSqls) {
        if (!q.exec(sql)) {
            db.rollback();
            qCritical() << "Create table failed:" << q.lastError().text() << "\nSQL:" << sql;
            return false;
        }
    }

    if (!db.commit()) {
        qCritical() << "Commit tables failed:" << db.lastError().text();
        return false;
    }

    // ========== 第二步：创建索引和触发器（事务保护） ==========
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction for indexes/triggers:" << db.lastError().text();
        return false;
    }

    // 创建索引（拆分SQL语句，跳过空行）
    const QStringList indexSqls = DatabaseSchema::getCreateIndexes().split(';', Qt::SkipEmptyParts);
    for (const QString& sql : indexSqls) {
        const QString trimmedSql = sql.trimmed();
        if (!trimmedSql.isEmpty() && !q.exec(trimmedSql)) {
            qWarning() << "Create index warning:" << q.lastError().text() << "\nSQL:" << trimmedSql;
            // 索引创建失败不中断（非核心错误）
        }
    }

    // 创建触发器
    const QStringList triggerSqls = DatabaseSchema::getCreateTriggers();
    for (const QString& sql : triggerSqls) {
        const QString trimmedSql = sql.trimmed();
        if (!trimmedSql.isEmpty() && !q.exec(trimmedSql)) {
            qWarning() << "Create trigger warning:" << q.lastError().text() << "\nSQL:" << trimmedSql;
            // 触发器创建失败不中断（非核心错误）
        }
    }

    if (!db.commit()) {
        qCritical() << "Commit indexes/triggers failed:" << db.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseInitializer::removeDatabaseFile()
{
    // 全局文件锁：防止多线程同时删除文件
    QMutexLocker lock(&s_fileMutex);
    if (m_dbPath.isEmpty()) return false;

    // 1. 关闭当前线程的数据库连接（由DbConnectionManager管理）
    auto dbPtr = DbConnectionManager::connectionForCurrentThread();
    if (dbPtr) {
        if (dbPtr->isOpen()) {
            dbPtr->close();  // 显式关闭连接
        }
        dbPtr.reset();      // 释放智能指针，确保文件句柄释放
    }

    // 2. 销毁所有可能的查询对象（防止句柄占用）
    QSqlQuery().finish();

    // 3. 延迟200ms：Windows下文件句柄释放可能有延迟
    QThread::msleep(200);

    // 4. 删除数据库文件（主文件 + WAL/SHM临时文件）
    QStringList filesToRemove = {
        m_dbPath,          // 主数据库文件
        m_dbPath + "-wal", // WAL日志文件
        m_dbPath + "-shm"  // 共享内存文件
    };

    bool allRemoved = true;
    for (const QString& filePath : filesToRemove) {
        QFile file(filePath);
        if (file.exists()) {
            if (!file.remove()) {
                qWarning() << "Failed to remove file:" << filePath;
                allRemoved = false;
            } else {
                qDebug() << "Removed file:" << filePath;
            }
        }
    }

    // 5. 重置初始化标记（删除文件后需要重新初始化）
    if (allRemoved) {
        s_initialized.store(false);
    }

    return allRemoved;
}
