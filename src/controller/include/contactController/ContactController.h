#ifndef CONTACTCONTROLLER_H
#define CONTACTCONTROLLER_H

#include <QObject>
#include <QMap>
#include "DatabaseManager.h"
#include "Contact.h"
#include "ContactTreeModel.h"


class ContactController : public QObject
{
    Q_OBJECT

public:
    explicit ContactController(DatabaseManager* dbManager, QObject* parent = nullptr);
    ~ContactController();
    ContactTreeModel *contactTreeModel() const{return m_contactTreeModel;}

    // 公共接口方法
    void addContact(int reqId, const Contact& contact);
    void updateContact(int reqId, const Contact& contact);
    void deleteContact(int reqId, qint64 userId);

    void getCurrentUser();
    void getContact(int reqId, qint64 userId);
    Contact getContactFromModel(qint64 userId);
    void getAllContacts(int reqId);
    void searchContacts(int reqId, const QString& keyword);
    void getStarredContacts(int reqId);

    void setContactStarred(int reqId, qint64 userId, bool starred);
    void setContactBlocked(int reqId, qint64 userId, bool blocked);

    void setSelectedContact(const Contact &contact);

signals:
    // 异步操作请求信号
    void addContactRequested(int reqId, const Contact& contact);
    void updateContactRequested(int reqId, const Contact& contact);
    void deleteContactRequested(int reqId, qint64 userId);
    void getContactRequested(int reqId, qint64 userId);
    void getCurrentUserRequested(int reqId);
    void getAllContactsRequested(int reqId);
    void searchContactsRequested(int reqId, const QString& keyword);
    void setContactStarredRequested(int reqId, qint64 userId, bool starred);
    void setContactBlockedRequested(int reqId, qint64 userId, bool blocked);
    void getStarredContactsRequested(int reqId);

    // 操作结果信号
    void currentUserLoaded(int reqId, const Contact& contact); // 当前用户加载结果
    void contactAdded(int reqId, bool success, const QString& error);
    void contactUpdated(int reqId, bool success, const QString& error);
    void contactDeleted(int reqId, bool success, const QString& error);
    void contactLoaded(int reqId, const Contact& contact);
    void allContactsLoaded(int reqId, const QList<Contact>& contacts);
    void searchContactsResult(int reqId, const QList<Contact>& contacts);
    void contactStarredChanged(int reqId, bool success, const QString& error);
    void contactBlockedChanged(int reqId, bool success, const QString& error);
    void starredContactsLoaded(int reqId, const QList<Contact>& contacts);
    void contactsChanged();

private slots:
    // 数据库操作结果处理槽函数
    void onContactSaved(int reqId, bool success, const QString& error);
    void onContactUpdated(int reqId, bool success, const QString& error);
    void onContactDeleted(int reqId, bool success, const QString& error);
    void onContactLoaded(int reqId, const Contact& contact);
    void onAllContactsLoaded(int reqId, const QList<Contact>& contacts);
    void onSearchContactsResult(int reqId, const QList<Contact>& contacts);
    void onContactStarredSet(int reqId, bool success);
    void onContactBlockedSet(int reqId, bool success);
    void onStarredContactsLoaded(int reqId, const QList<Contact>& contacts);

private:
    void connectSignals();
    void disconnectSignals();
    void connectAsyncSignals();
    int generateReqId();   // 生成唯一请求ID


private:
    DatabaseManager* m_dbManager;
    ContactTable* m_contactTable;
    QAtomicInteger<int> m_reqIdCounter;      // 请求ID计数器（线程安全）
    QHash<int, QString> m_pendingUpdates; // 待处理操作（reqId->操作类型）
    ContactTreeModel *m_contactTreeModel;
    Contact m_contact;

};

#endif // CONTACTCONTROLLER_H
