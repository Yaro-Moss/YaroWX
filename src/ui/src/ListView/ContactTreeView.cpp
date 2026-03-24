#include "ContactTreeView.h"
#include "ClickClosePopup.h"
#include "ContactTreeModel.h"
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QDebug>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>


ContactTreeView::ContactTreeView(QWidget *parent)
    : CustomTreeView(parent)
    , m_contactModel(nullptr)
{
    // 配置视图
    setHeaderHidden(true);
    setSelectionMode(SingleSelection);
    setAlternatingRowColors(false);
    setRootIsDecorated(true);
    setUniformRowHeights(false);
    setEditTriggers(QTreeView::NoEditTriggers);

    createContactMenu();
    // 展开所有节点
    expandAll();

}


void ContactTreeView::setContactModel(ContactTreeModel *model)
{
    m_contactModel = model;
    setModel(model);
}

Contact ContactTreeView::getSelectedContact() const
{
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (!selectedIndexes.isEmpty()) {
        return getContactFromIndex(selectedIndexes.first());
    }
    return Contact();
}

Contact ContactTreeView::getContactFromIndex(const QModelIndex &index) const
{
    if (!index.isValid()) return Contact();

    QVariant contactData = index.data(Qt::UserRole);
    if (contactData.isValid() && contactData.canConvert<Contact>()) {
        return contactData.value<Contact>();
    }
    return Contact();
}

void ContactTreeView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (m_contactModel && m_contactModel->isParentNode(index)) {
        // 对于父节点
        handleParentNodeClick(index);
        event->accept(); // 处理事件，不进行选择
        return;

    } else {
        // 对于其他节点，正常处理
        CustomTreeView::mousePressEvent(event);
    }
}

void ContactTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (m_contactModel && m_contactModel->isParentNode(index)) {
        // 对于父节点，确保不会选中
        clearSelection();
        event->accept();
    } else {
        CustomTreeView::mouseReleaseEvent(event);
    }
}

void ContactTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (m_contactModel && m_contactModel->isParentNode(index)) {
        // 对于父节点，双击也不选中，但可以切换展开状态
        if (isExpanded(index)) {
            collapse(index);
        } else {
            expand(index);
        }
        event->accept();
    } else {
        CustomTreeView::mouseDoubleClickEvent(event);
    }
}


void ContactTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        return;
    }
    Contact contact = getSelectedContact();
    if (contact.isValid()) {

        starFriendAction->setText(contact.is_starredValue() ? "取消星标朋友" : "标为星标朋友");
        contactMenu->exec(event->globalPos());
    }
}


void ContactTreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    // 首先检查是否有选择，并且选中的是父节点
    if (m_contactModel && !selected.isEmpty()) {
        QModelIndexList selectedIndexes = selected.indexes();
        if (!selectedIndexes.isEmpty()) {
            QModelIndex selectedIndex = selectedIndexes.first();
            if (m_contactModel->isParentNode(selectedIndex)) {
                // 清除对父节点的选择
                clearSelection();
                return; // 直接返回，不调用基类
            }
        }
    }

    // 正常处理选择变化
    CustomTreeView::selectionChanged(selected, deselected);
}

void ContactTreeView::handleParentNodeClick(const QModelIndex &index)
{
    // 切换展开状态
    if (isExpanded(index)) {
        collapse(index);
    } else {
        expand(index);
    }
}


void ContactTreeView::createContactMenu()
{
    contactMenu = new QMenu(this);
    contactMenu->setStyleSheet(R"(
        QMenu {
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 6px;
            padding: 4px;
            font-family: "Microsoft YaHei";
            font-size: 14px;
        }
        QMenu::item {
            height: 32px;
            padding: 0px 12px;
            border-radius: 4px;
            margin: 2px;
            color: #333333;
        }
        QMenu::item:selected:!disabled {
            background-color: #4CAF50;
            color: white;
        }
        QMenu::item[class="delete"] {
            color: #ff4444;
        }
        QMenu::item[class="delete"]:selected:!disabled {
            background-color: #ff4444;
            color: white;
        }
        QMenu::separator {
            height: 1px;
            background: #e0e0e0;
            margin: 4px 8px;
        }
    )");

    sendMessageAction = new QAction("发消息", this);
    starFriendAction = new QAction("标为星标朋友", this);
    removeFriendAction = new QAction("删除朋友", this);
    removeFriendAction->setProperty("class", "delete");

    contactMenu->addAction(sendMessageAction);
    contactMenu->addAction(starFriendAction);
    contactMenu->addAction(removeFriendAction);

    // 连接信号
    connect(sendMessageAction, &QAction::triggered, this, [this]() {
        emit sendMessage(getSelectedContact());
    });

    connect(starFriendAction, &QAction::triggered, this, [this]() {
        emit starFriend(getSelectedContact());
    });

    connect(removeFriendAction, &QAction::triggered, this, [this]() {
        showDeleteConfirmationDialog();
    });
}


void ContactTreeView::showDeleteConfirmationDialog()
{
    // 创建自定义确认对话框
    ClickClosePopup* confirmationDialog = new ClickClosePopup(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(confirmationDialog);

    // 添加提示文本
    QLabel* messageLabel = new QLabel("删除该朋友");
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setStyleSheet("font-family: 'Microsoft YaHei'; font-size: 14px; color: #333333; "
                                "padding: 10px; border: none; background-color: transparent;");
    // 创建按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* deleteButton = new QPushButton("删除");
    QPushButton* cancelButton = new QPushButton("取消");

    // 设置按钮样式
    deleteButton->setStyleSheet(R"(
        QPushButton {
            background-color: #ff4444;
            color: white;
            border: none;
            border-radius: 4px;
            min-width: 80px;
            min-height: 30px;
            font-family: "Microsoft YaHei";
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #cc3333;
        }
        QPushButton:pressed {
            background-color: #aa2222;
        }
    )");

    cancelButton->setStyleSheet(R"(
        QPushButton {
            background-color: #f0f0f0;
            color: #333333;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            min-width: 80px;
            min-height: 30px;
            font-family: "Microsoft YaHei";
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #e0e0e0;
        }
        QPushButton:pressed {
            background-color: #d0d0d0;
        }
    )");

    // 设置按钮布局
    buttonLayout->addStretch();
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    // 设置主布局
    mainLayout->addWidget(messageLabel);
    mainLayout->addLayout(buttonLayout);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    // 连接按钮信号
    connect(deleteButton, &QPushButton::clicked, confirmationDialog, [this, confirmationDialog]() {
        confirmationDialog->close();
        emit removeFriend(getSelectedContact());
    });

    connect(cancelButton, &QPushButton::clicked, confirmationDialog, &ClickClosePopup::close);

    // 显示对话框
    confirmationDialog->show();
    confirmationDialog->adjustSize();
}

