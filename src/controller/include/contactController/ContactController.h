#ifndef CONTACTCONTROLLER_H
#define CONTACTCONTROLLER_H

#include <QObject>
#include "Contact.h"
#include "ContactTreeModel.h"
class Network;
class NetworkDataLoader;

class ContactController : public QObject
{
    Q_OBJECT

public:
    explicit ContactController(Network *network, QObject* parent = nullptr);
    ~ContactController();

    ContactTreeModel *contactTreeModel() const { return m_contactTreeModel; }

    // 公共接口（均为异步操作）
    Q_INVOKABLE void getAllContacts();
    Q_INVOKABLE void searchUser(const QString& keyword);
    Q_INVOKABLE void getContact(qint64 userId);
    Q_INVOKABLE void addContact(const Contact& contact);
    Q_INVOKABLE void updateContact(const Contact& contact);
    Q_INVOKABLE void deleteContact(qint64 userId);
    Q_INVOKABLE void sendFriendRequest(QString &errorMessage,
                                       qint64 to_user_id,
                                       qint64 &outRequestId,
                                       const QString& message = QString(),
                                       const QString& remark = QString(),
                                       const QString& tags = QString(),
                                       const QString& source = QString(),
                                       const QString& description = QString());


    // 设置状态（异步）
    Q_INVOKABLE void setContactStarred(qint64 userId, bool starred);
    Q_INVOKABLE void setContactBlocked(qint64 userId, bool blocked);

    // 同步获取（从模型）
    Q_INVOKABLE Contact getContactFromModel(qint64 userId);
    Q_INVOKABLE Contact getCurrentLoginUser();

    // 设置当前选中的联系人（用于 UI）
    void setSelectedContact(const Contact &contact);

signals:
    void contactsLoaded();              // 联系人列表加载完成
    void contactUpdated(qint64 userId); // 某个联系人被更新
    void contactRemoved(qint64 userId); // 某个联系人被删除
    void searchUsered(const QVector<Contact>& Contacts); // 搜索用户

private:
    ContactTreeModel *m_contactTreeModel;
    Contact m_selectedContact;          // 当前选中的联系人
    NetworkDataLoader *m_networkDataLoader;
};

#endif // CONTACTCONTROLLER_H
