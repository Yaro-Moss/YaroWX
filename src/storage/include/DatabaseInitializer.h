#pragma once

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QMutex>
#include <atomic>

class DatabaseInitializer : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseInitializer(QObject* parent = nullptr);
    ~DatabaseInitializer() override;

    bool ensureInitialized();

    static QString databasePath();

    static void applyPragmas(QSqlDatabase &db);

    bool removeDatabaseFile();

private:
    bool createTables(QSqlDatabase &db);

    bool databaseFileExists() const;

    // 成员变量
    QString m_dbPath;                          // 数据库文件路径
    static std::atomic<bool> s_initialized;    // 初始化状态标记（原子变量，线程安全）
    static QMutex s_initMutex;                 // 初始化互斥锁
    static QMutex s_fileMutex;                 // 文件操作互斥锁（防止多线程删文件）
};
