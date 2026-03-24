#ifndef CONTACTCONTROLLER_H
#define CONTACTCONTROLLER_H

#include <QObject>
#include "Contact.h"
#include "ContactTreeModel.h"

class ContactController : public QObject
{
    Q_OBJECT

public:
    explicit ContactController(QObject* parent = nullptr);
    ~ContactController();

    ContactTreeModel *contactTreeModel() const { return m_contactTreeModel; }

    // 公共接口（均为异步操作）
    Q_INVOKABLE void getAllContacts();
    Q_INVOKABLE void searchContacts(const QString& keyword);
    Q_INVOKABLE void getContact(qint64 userId);
    Q_INVOKABLE void addContact(const Contact& contact);
    Q_INVOKABLE void updateContact(const Contact& contact);
    Q_INVOKABLE void deleteContact(qint64 userId);

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

private:
    ContactTreeModel *m_contactTreeModel;
    Contact m_selectedContact;          // 当前选中的联系人
};

#endif // CONTACTCONTROLLER_H
