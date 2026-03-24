#ifndef WECHATWIDGET_H
#define WECHATWIDGET_H

#include <QWidget>
#include <QPointer>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include "Contact.h"
#include "Conversation.h"
#include <QTreeView>

namespace Ui {
class WeChatWidget;
}
class QSplitter;
class QFrame;
class RightPopover;
class AddDialog ;
class MoreDialog;
class FloatingDialog;
class CurrentUserInfoDialog;
class MediaDialog;
class ChatMessageListView;
class ChatListDelegate;
class ChatMessageDelegate;
class ChatMessagesModel;
class ConversationController;
class MessageController;
class AppController;
class ChatListView;
class ChatListModel;
class UserController;
class AudioPlayer;
class ContactTreeView;
class ContactItemDelegate;
class ContactController;
class UserInfoWidget;
class MomentMainWidget;
class LocalMomentController;

class WeChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WeChatWidget(AppController *Controller, QWidget *parent = nullptr);
    ~WeChatWidget();

public slots:
    void on_switchtoMessageInterface(const Contact &contact);


protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:

    void on_contactsToolButton_clicked();

    void on_collectionToolButton_clicked();

    void on_rightDialogToolButton_clicked();

    void rightStackedWidgetPageSizeChange();

    void on_addToolButton_clicked();

    void on_moreToolButton_clicked();

    void on_floatingToolButton_clicked();

    void on_avatarPushButton_clicked();

    void on_chatInterfaceToolButton_clicked();

    void on_closeButton_clicked();

    void on_maxWinButton_clicked();

    void on_minWinButton_clicked();

    void on_pinButton_clicked();

    void on_selectFileButton_clicked();

    void on_sendPushButton_clicked();

    void on_recordVoiceButton_clicked();

    void on_momentButton_clicked();

private:
    //自定义窗口相关
    bool m_isOnTop; // 记录当前是否置顶
    int m_titleBarHeight;
    bool m_isMaximized;
    bool m_isDragging;
    bool m_isDraggingMax;
    enum Edge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };
    Edge m_currentEdge;
    bool m_isResizing;
    int  m_borderWidth;
    QRect m_windowGeometry;       // 用于存储窗口几何信息
    QPoint m_dragStartPosition;   // 用于存储鼠标拖动起始位置
    // 辅助函数
    Edge getEdge(const QPoint &pos);
    void updateCursorShape(const QPoint &pos);
    void handleResize(const QPoint &currentGlobalPos);// 处理拉伸（参数为当前鼠标全局坐标）
    void handleDrag(const QPoint &currentGlobalPos);// 处理移动（参数为当前鼠标全局坐标）


    Ui::WeChatWidget *ui;
    //弹窗
    QPointer<RightPopover> rightPopover;
    QPointer<AddDialog> addDialog;
    QPointer<MoreDialog> moreDialog;
    QPointer<FloatingDialog> floatingDialog;
    QPointer<MediaDialog> mediaDialog;
    QPointer<MomentMainWidget>momentMainWidget;
    LocalMomentController* localMomentController;

    UserInfoWidget *userInfoWidget;

    AudioPlayer *audioPlayer;  // 音频播放

    UserController *userController;

    //聊天会话列表
    ConversationController *conversationController;
    ChatListDelegate *chatListDelegate;
    ChatListView *chatListView;

    // 聊天消息页
    MessageController *messageController;
    ChatMessageDelegate *chatMessageDelegate;
    ChatMessageListView *chatMessageListView;

    Conversation currentConversation;
    Contact m_currentLoginUser;
    Contact m_contact;

    // 通讯录TreeView
    ContactController *contactController;
    ContactTreeView *contactTreeView;
    ContactItemDelegate *contactItemDelegate;

    void updateSendButtonStyle();//更新发送按钮样式

    void initChatList();
    void initMessageList();
    void initCurrentUser();
    void initContactList();

};

#endif // WECHATWIDGET_H
