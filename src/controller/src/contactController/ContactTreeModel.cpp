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
    // 创建父节点并设置不可选中但可展开
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

    // 添加联系人分组节点
    m_contactGroupItem = new QStandardItem("联系人");
    m_contactGroupItem->setFlags(parentFlags);
    appendRow(m_contactGroupItem);
}

void ContactTreeModel::loadContacts(const QList<Contact>& contacts)
{
    if (!m_contactGroupItem) return;

    // 清空现有联系人
    if (m_contactGroupItem->rowCount() > 0) {
        m_contactGroupItem->removeRows(0, m_contactGroupItem->rowCount());
    }

    // 设置联系人节点的标志 - 可选中
    Qt::ItemFlags contactFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    // 添加联系人
    for (const Contact &contact : contacts) {
        QStandardItem *contactItem = new QStandardItem();

        // 设置联系人项为可选中
        contactItem->setFlags(contactFlags);

        // 将联系人数据存储为UserRole
        contactItem->setData(QVariant::fromValue(contact), Qt::UserRole);

        m_contactGroupItem->appendRow(contactItem);
    }
}

void ContactTreeModel::addContact(const Contact& contact)
{
    if (!m_contactGroupItem) return;

    // 设置联系人节点的标志 - 可选中
    Qt::ItemFlags contactFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    QStandardItem *contactItem = new QStandardItem();
    contactItem->setFlags(contactFlags);
    contactItem->setData(QVariant::fromValue(contact), Qt::UserRole);

    m_contactGroupItem->appendRow(contactItem);
}

bool ContactTreeModel::isParentNode(const QModelIndex &index) const
{
    if (!index.isValid()) return false;

    QStandardItem *item = itemFromIndex(index);
    if (!item) return false;

    // 检查是否是固定的父节点
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

    // 检查是否有联系人数据
    QVariant contactData = item->data(Qt::UserRole);
    return contactData.isValid() && contactData.canConvert<Contact>();
}


QModelIndex ContactTreeModel::findContactIndex(qint64 userId) const
{
    if (!m_contactGroupItem) return QModelIndex();

    for (int row = 0; row < m_contactGroupItem->rowCount(); ++row) {
        QStandardItem* contactItem = m_contactGroupItem->child(row);
        Contact contact = contactItem->data(Qt::UserRole).value<Contact>();

        if (contact.userId == userId) {
            return contactItem->index();
        }
    }
    return QModelIndex();
}

Contact ContactTreeModel::getContactById(qint64 userId) const
{
    Contact emptyContact;
    emptyContact.userId = -1;

    // 校验分组节点有效性
    if (!m_contactGroupItem) {
        return emptyContact;
    }

    // 遍历查找联系人
    for (int row = 0; row < m_contactGroupItem->rowCount(); ++row) {
        QStandardItem* contactItem = m_contactGroupItem->child(row);
        if (!contactItem) {
            continue;
        }

        // 取出并校验Contact数据
        QVariant contactVar = contactItem->data(Qt::UserRole);
        if (!contactVar.isValid() || !contactVar.canConvert<Contact>()) {
            continue;
        }

        Contact contact = contactVar.value<Contact>();
        if (contact.userId == userId) {
            return contact;
        }
    }

    return emptyContact;
}
