#ifndef LOCALMOMENTTABLE_H
#define LOCALMOMENTTABLE_H

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QVector>
#include "LocalMoment.h"

class LocalMomentTable : public QObject
{
    Q_OBJECT

public:
    explicit LocalMomentTable(QObject* parent = nullptr);
    ~LocalMomentTable() override;


public slots:
    void init();
    // （reqId为请求ID，用于关联请求和响应）
    void saveMoment(int reqId, const LocalMoment& moment);
    void deleteMoment(int reqId, qint64 momentId);
    void getMoments(int reqId, int limit = 30, int offset = 0);
    void getMomentById(int reqId, qint64 momentId);
    void getMomentInteract(int reqId, qint64 momentId);
    void saveMomentInteract(int reqId, const LocalMomentInteract& interact);
    void updateMediaDownloadStatus(int reqId, qint64 momentId, bool isVideo, const QString& mediaUrl, const QString& localPath, int status);
    void clearExpiredMoments(int reqId);

signals:
    // 操作结果信号
    void momentSaved(int reqId, bool ok, const QString& reason = QString());
    void momentDeleted(int reqId, bool ok, const QString& reason = QString());
    void momentsLoaded(int reqId, const QVector<LocalMoment>& moments);
    void momentLoaded(int reqId, const LocalMoment& moment);
    void momentInteractLoaded(int reqId, const LocalMomentInteract& interact);
    void expiredMomentsCleared(int reqId, int count, const QString& reason = QString());
    void dbError(int reqId, const QString& error);

private:
    bool getMomentByIdInternal(qint64 momentId, LocalMoment& outMoment);

    QSharedPointer<QSqlDatabase> m_database = nullptr;
};

#endif // LOCALMOMENTTABLE_H
