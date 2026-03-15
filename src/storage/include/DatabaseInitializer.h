#pragma once

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QMutex>
#include <atomic>

class DatabaseInitializer : public QObject {
    Q_OBJECT
public:
    explicit DatabaseInitializer(QObject* parent = nullptr);
    ~DatabaseInitializer() override;

    bool ensureInitialized();

    static QString databasePath();

    // 将 PRAGMA 应用于传入的连接（供外部线程连接复用）
    static void applyPragmas(QSqlDatabase &db);

private:
    bool databaseFileExists() const;
    bool openMainConnection();
    bool createTables(QSqlDatabase &db);

    bool removeDatabaseFile();

    QString m_dbPath;

    static std::atomic<bool> s_initialized;
    static QMutex s_initMutex;
};
