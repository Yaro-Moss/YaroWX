#include "ChatListView.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include "Conversation.h"
#include <QMessageBox>
#include <QPushButton>
#include "ClickClosePopup.h"
#include <QVBoxLayout>
#include <QLabel>

ChatListView::ChatListView(QWidget *parent)
    : CustomListView(parent), m_currentConversationId(0)
{
    createConversationContextMenu();

    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection); // 单选
}

ChatListView::~ChatListView()
{
    delete m_conversationMenu;
}


void ChatListView::createConversationContextMenu()
{
    m_conversationMenu = new QMenu(this);

    // 设置菜单样式
    m_conversationMenu->setStyleSheet(R"(
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

    m_toggleTopAction = new QAction("置顶", this);
    m_markAsUnreadAction = new QAction("标为未读", this);
    m_toggleMuteAction = new QAction("消息免打扰", this);
    m_openInWindowAction = new QAction("独立窗口显示", this);
    m_deleteAction = new QAction("删除", this);

    // 为删除项设置特殊类名
    m_deleteAction->setProperty("class", "delete");

    m_conversationMenu->addAction(m_toggleTopAction);
    m_conversationMenu->addAction(m_markAsUnreadAction);
    m_conversationMenu->addAction(m_toggleMuteAction);
    m_conversationMenu->addSeparator();
    m_conversationMenu->addAction(m_openInWindowAction);
    m_conversationMenu->addSeparator();
    m_conversationMenu->addAction(m_deleteAction);

    // 连接信号
    connect(m_toggleTopAction, &QAction::triggered, this, [this]() {
        emit conversationToggleTop(m_currentConversationId);
    });
    connect(m_markAsUnreadAction, &QAction::triggered, this, [this]() {
        emit conversationMarkAsUnread(m_currentConversationId);

    });
    connect(m_toggleMuteAction, &QAction::triggered, this, [this]() {
        emit conversationToggleMute(m_currentConversationId);
    });
    connect(m_openInWindowAction, &QAction::triggered, this, [this]() {
        emit conversationOpenInWindow(m_currentConversationId);
    });
    connect(m_deleteAction, &QAction::triggered, this, [this]() {
        showDeleteConfirmationDialog();
    });
}

void ChatListView::showDeleteConfirmationDialog()
{
    // 创建自定义确认对话框
    ClickClosePopup* confirmationDialog = new ClickClosePopup(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(confirmationDialog);

    // 添加提示文本
    QLabel* messageLabel = new QLabel("删除该聊天会话？");
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
        emit conversationDelete(m_currentConversationId);
    });

    connect(cancelButton, &QPushButton::clicked, confirmationDialog, &ClickClosePopup::close);

    // 显示对话框
    confirmationDialog->show();
    confirmationDialog->adjustSize();
}


void ChatListView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        return;
    }

    m_currentConversationId = index.data(ConversationIdRole).toLongLong();
    if (m_currentConversationId != 0) {

        bool isTop = index.data(IsTopRole).toBool();
        m_toggleTopAction->setText(isTop ? "取消置顶" : "置顶");

        int unreadCount = index.data(UnreadCountRole).toInt();
        m_markAsUnreadAction->setText(unreadCount > 0 ? "标记为已读" : "标记为未读");

        m_conversationMenu->exec(event->globalPos());
    }
}


Conversation ChatListView::getConversationFromIndex(const QModelIndex &index) const
{
    Conversation conversationInfo;

    if (!index.isValid()) {
        return conversationInfo;
    }

    conversationInfo.setconversation_id(index.data(ConversationIdRole).toLongLong());
    conversationInfo.setgroup_id(index.data(GroupIdRole).toLongLong());
    conversationInfo.setuser_id(index.data(UserIdRole).toLongLong());
    conversationInfo.settype(index.data(TypeRole).toInt());
    conversationInfo.settitle(index.data(TitleRole).toString());
    conversationInfo.setavatar(index.data(AvatarRole).toString());
    conversationInfo.setavatar_local_path(index.data(AvatarLocalPathRole).toString());
    conversationInfo.setlast_message_content(index.data(LastMessageContentRole).toString());
    conversationInfo.setlast_message_time(index.data(LastMessageTimeRole).toLongLong());
    conversationInfo.setunread_count(index.data(UnreadCountRole).toInt());
    conversationInfo.setis_top(index.data(IsTopRole).toBool());

    return conversationInfo;
}


Conversation ChatListView::getSelectedConversation() const
{
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (!selectedIndexes.isEmpty()) {
        return getConversationFromIndex(selectedIndexes.first());
    }
    return Conversation();
}






