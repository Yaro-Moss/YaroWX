#include "ContactTreeModel.h"

ContactTreeModel::ContactTreeModel(QObject *parent)
    : QStandardItemModel(parent)
    , m_newFriendItem(nullptr)
    , m_officialAccountItem(nullptr)
    , m_serviceAccountItem(nullptr)
    , m_contactGroupItem(nullptr)
{
    setupFixedNodes();
}

void ContactTreeModel::setupFixedNodes()
{
    Qt::ItemFlags parentFlags = Qt::ItemIsEnabled;

    m_newFriendItem = new QStandardItem("新的朋友");
    m_newFriendItem->setFlags(parentFlags);
    appendRow(m_newFriendItem);

    m_officialAccountItem = new QStandardItem("公众号");
    m_officialAccountItem->setFlags(parentFlags);
    appendRow(m_officialAccountItem);

    m_serviceAccountItem = new QStandardItem("服务号");
    m_serviceAccountItem->setFlags(parentFlags);
    appendRow(m_serviceAccountItem);

    m_contactGroupItem = new QStandardItem("联系人");
    m_contactGroupItem->setFlags(parentFlags);
    appendRow(m_contactGroupItem);
}

void ContactTreeModel::loadContacts(const QList<Contact>& contacts)
{
    if (!m_contactGroupItem) return;

    // 清空现有联系人（同时清空哈希表，因为子项将被销毁）
    if (m_contactGroupItem->rowCount() > 0) {
        m_contactGroupItem->removeRows(0, m_contactGroupItem->rowCount());
        m_contactItems.clear();  // 所有 item 已销毁，哈希表必须清空
    }

    Qt::ItemFlags contactFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    for (const Contact &contact : contacts) {
        QStandardItem *contactItem = new QStandardItem();
        contactItem->setFlags(contactFlags);
        contactItem->setData(QVariant::fromValue(contact), Qt::UserRole);

        // 插入哈希表
        m_contactItems.insert(contact.userId, contactItem);

        m_contactGroupItem->appendRow(contactItem);
    }
}

void ContactTreeModel::addContact(const Contact& contact)
{
    if (!m_contactGroupItem) return;

    // 检查是否已存在相同 userId 的联系人
    if (m_contactItems.contains(contact.userId)) {
        // 已存在 更新现有 item 的数据（保持指针不变，哈希表无需改动）
        QStandardItem *existingItem = m_contactItems.value(contact.userId);
        existingItem->setData(QVariant::fromValue(contact), Qt::UserRole);
        return;
    }

    Qt::ItemFlags contactFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    QStandardItem *contactItem = new QStandardItem();
    contactItem->setFlags(contactFlags);
    contactItem->setData(QVariant::fromValue(contact), Qt::UserRole);

    // 插入哈希表
    m_contactItems.insert(contact.userId, contactItem);

    m_contactGroupItem->appendRow(contactItem);
}

bool ContactTreeModel::isParentNode(const QModelIndex &index) const
{
    if (!index.isValid()) return false;
    QStandardItem *item = itemFromIndex(index);
    if (!item) return false;

    return (item == m_newFriendItem ||
            item == m_officialAccountItem ||
            item == m_serviceAccountItem ||
            item == m_contactGroupItem);
}

bool ContactTreeModel::isContactNode(const QModelIndex &index) const
{
    if (!index.isValid()) return false;
    QStandardItem *item = itemFromIndex(index);
    if (!item) return false;

    QVariant contactData = item->data(Qt::UserRole);
    return contactData.isValid() && contactData.canConvert<Contact>();
}

QModelIndex ContactTreeModel::findContactIndex(qint64 userId) const
{
    QStandardItem *item = m_contactItems.value(userId, nullptr);
    if (item) {
        return item->index();
    }
    return QModelIndex();
}

Contact ContactTreeModel::getContactById(qint64 userId) const
{
    QStandardItem *item = m_contactItems.value(userId, nullptr);
    if (item) {
        QVariant contactVar = item->data(Qt::UserRole);
        if (contactVar.isValid() && contactVar.canConvert<Contact>()) {
            return contactVar.value<Contact>();
        }
    }
    // 返回一个表示无效的联系人
    Contact emptyContact;
    emptyContact.userId = -1;
    return emptyContact;
}
