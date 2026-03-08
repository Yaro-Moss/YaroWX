#include "WeChatWidget.h"
#include "AudioPlayer.h"
#include "ContactItemDelegate.h"
#include "MomentWidgets.h"
#include "ui_WeChatWidget.h"
#include "RightPopover.h"
#include "AddDialog.h"
#include "MoreDialog.h"
#include "FloatingDialog.h"
#include "CurrentUserInfoDialog.h"
#include "MediaDialog.h"
#include "ImgLabel.h"
#include "ChatListDelegate.h"
#include "ChatMessageDelegate.h"
#include "AppController.h"
#include <QSplitter>
#include <QFrame>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCheckBox>
#include <QFileDialog>
#include "MessageTextEdit.h"
#include <QMessageBox>
#include "ChatListView.h"
#include "ChatMessageListView.h"
#include "VoiceRecordDialog.h"


WeChatWidget::WeChatWidget(AppController * appController, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WeChatWidget)
    // 弹窗
    , addDialog(nullptr)
    , moreDialog(nullptr)
    , rightPopover(nullptr)
    , floatingDialog(nullptr)
    , mediaDialog(new MediaDialog())
    , momentMainWidget(nullptr)


    // 自定义窗口相关
    , m_isOnTop(false)
    , m_titleBarHeight(70)
    , m_isMaximized(false)
    , m_isDragging(false)
    , m_isDraggingMax(false)
    , m_currentEdge(None)
    , m_isResizing(false)
    , m_borderWidth(5)

    // 控制器
    , conversationController(appController->conversationController())
    , messageController(appController->messageController())
    , userController(appController->userController())
    , contactController(appController->contactController())
    , localMomentController(appController->localMomentController())

    , audioPlayer(AudioPlayer::instance())

{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    ui->rightStackedWidgetPage0->hide();
    ui->rightStackedWidgetpage1->hide();

    initChatList();
    initMessageList();
    initCurrentUser();
    initContactList();

    //检查信息输入框状态，设置初始样式、连接信号
    updateSendButtonStyle();
    connect(ui->sendTextEdit, &QTextEdit::textChanged,
            this, &WeChatWidget::updateSendButtonStyle);
    connect(ui->sendTextEdit, &MessageTextEdit::returnPressed, this,
            &WeChatWidget::on_sendPushButton_clicked);

    qApp->installEventFilter(this);
}

// 初始化会话列表
void WeChatWidget::initChatList()
{
    chatListView = ui->chatListView;
    chatListDelegate = new ChatListDelegate(chatListView);
    chatListDelegate->setContactController(contactController);
    chatListView->setItemDelegate(chatListDelegate);
    chatListView->setModel(conversationController->chatListModel());

    connect(chatListView, &ChatListView::conversationToggleTop,
            conversationController, &ConversationController::handleToggleTop);

    connect(chatListView, &ChatListView::conversationMarkAsUnread,
            conversationController, &ConversationController::handltoggleReadStatus);

    connect(chatListView, &ChatListView::conversationToggleMute,
            conversationController, &ConversationController::handleToggleMute);

    connect(chatListView, &ChatListView::conversationOpenInWindow,
            conversationController, &ConversationController::handleOpenInWindow);

    connect(chatListView, &ChatListView::conversationDelete,
            conversationController, &ConversationController::handleDelete);

    // 会话列表选中项改变时触发
    connect(chatListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected, const QItemSelection &deselected){
                if (!selected.isEmpty()) {

                    if(chatListView->getSelectedConversation().isValid())
                        currentConversation = chatListView->getSelectedConversation();

                    // 会话显示
                    ui->chatPartnerLabel->setText(currentConversation.title);
                    ui->rightStackedWidget->setCurrentWidget(ui->rightStackedWidgetPage0);
                    ui->rightStackedWidgetPage0->show();

                    messageController->setCurrentConversation(currentConversation);
                    conversationController->setCurrentConversationId(currentConversation.conversationId);
                    QTimer::singleShot(100, this, [=]() {
                        if (chatMessageListView != nullptr && !chatMessageListView->isHidden()) {
                            chatMessageListView->scrollToBottom();
                        }
                    });

                    // 标记已读
                    conversationController->clearUnreadCount(currentConversation.conversationId);
                } else if (!deselected.isEmpty()) {
                    ui->rightStackedWidgetPage0->hide();
                }
            });
    // 处理其他业务如顶置聊天时重新加载，选中加载前的选中项
    connect(conversationController, &ConversationController::conversationLoaded, this,
            [this](QString functionCaller){
                if(!functionCaller.isEmpty() && functionCaller=="createSingleChat"){
                    qDebug()<<"createSingleChat";
                    on_switchtoMessageInterface(m_contact);
                    return;
                }

                if (currentConversation.isValid()) {
                    ChatListModel *m_model = conversationController->chatListModel();
                    QModelIndex index = m_model->getConversationIndex(currentConversation.conversationId);

                    if (index.isValid()) {
                        chatListView->setCurrentIndex(index);
                        chatListView->scrollTo(index);
                    }
                }
            });
    // 加载会话列表
    conversationController->loadConversations(0);
}

// 初始化消息列表
void WeChatWidget::initMessageList()
{
    chatMessageListView = ui->messageListView;
    chatMessageDelegate = new ChatMessageDelegate(chatMessageListView);
    chatMessageDelegate->setContactController(contactController);
    chatMessageListView->setModel(messageController->messagesModel());
    chatMessageListView->setItemDelegate(chatMessageDelegate);

    connect(chatMessageListView, &ChatMessageListView::messageCopy,
            messageController, &MessageController::handleCopy);

    connect(chatMessageListView, &ChatMessageListView::messageZoom,
            messageController, &MessageController::handleZoom);

    connect(chatMessageListView, &ChatMessageListView::messageTranslate,
            messageController, &MessageController::handleTranslate);

    connect(chatMessageListView, &ChatMessageListView::messageSearch,
            messageController, &MessageController::handleSearch);

    connect(chatMessageListView, &ChatMessageListView::messageForward,
            messageController, &MessageController::handleForward);

    connect(chatMessageListView, &ChatMessageListView::messageFavorite,
            messageController, &MessageController::handleFavorite);

    connect(chatMessageListView, &ChatMessageListView::messageRemind,
            messageController, &MessageController::handleRemind);

    connect(chatMessageListView, &ChatMessageListView::messageMultiSelect,
            messageController, &MessageController::handleMultiSelect);

    connect(chatMessageListView, &ChatMessageListView::messageQuote,
            messageController, &MessageController::handleQuote);

    connect(chatMessageListView, &ChatMessageListView::messageDelete,
            messageController, &MessageController::handleDelete);

    connect(chatMessageListView, &ChatMessageListView::loadmoreMsg,
            messageController,&MessageController::loadMoreMessages);

    // 保存消息时刷新会话列表，（最后一条消息和时间。）
    connect(messageController, &MessageController::messageSaved, this, [this](){
        chatMessageListView->scrollToBottom();
        conversationController->loadConversations(0);
    });

    // 点击消息时信号处理
    connect(chatMessageDelegate, &ChatMessageDelegate::rightClicked, chatMessageListView,
            &ChatMessageListView::execMessageListMenu);

    connect(chatMessageDelegate, &ChatMessageDelegate::mediaClicked, this,
            [this](const qint64 msgId, const qint64 conversationId){
                qDebug()<<"点击媒体";
                if(!mediaDialog)
                {
                    mediaDialog = new MediaDialog();
                }
                disconnect(messageController, &MessageController::mediaItemsLoaded, this, nullptr); //避免重复连接
                connect(messageController, &MessageController::mediaItemsLoaded, this,
                        [this,msgId](const QList<MediaItem>& items){
                            if (mediaDialog) {
                                mediaDialog->setMediaItems(items);
                                mediaDialog->selectMediaByMessageId(msgId);
                            }
                        });

                messageController->getMediaItems(conversationId);
                mediaDialog->close();
                mediaDialog->show();
            });

    connect(chatMessageDelegate, &ChatMessageDelegate::fileClicked, this, [this](const QString &filePath){
        qDebug()<<"点击文件";
        bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    });

    connect(chatMessageDelegate, &ChatMessageDelegate::voiceClicked, this,
            [this](const QString &voicePath, const qint64 &messageId) {
                audioPlayer->play(voicePath, messageId);
            });

    connect(audioPlayer, &AudioPlayer::playbackStarted, chatMessageListView,
            [this]() { chatMessageListView->viewport()->update(); });
    connect(audioPlayer, &AudioPlayer::playbackPaused, chatMessageListView,
            [this]() { chatMessageListView->viewport()->update(); });
    connect(audioPlayer, &AudioPlayer::playbackStopped, chatMessageListView,
            [this]() { chatMessageListView->viewport()->update(); });
    connect(audioPlayer, &AudioPlayer::playbackFinished, chatMessageListView,
            [this]() { chatMessageListView->viewport()->update(); });
    connect(audioPlayer, &AudioPlayer::waveformUpdated, chatMessageListView,
            [this]() { chatMessageListView->viewport()->update(); });
    connect(chatMessageDelegate, &ChatMessageDelegate::avatarClicked, this,
        [this](const qint64 &senderId){
                Contact contact = contactController->getContactFromModel(senderId);
                if(contact.user.isCurrent){
                    CurrentUserInfoDialog *currentUserInfoDialog = new CurrentUserInfoDialog(this);
                    currentUserInfoDialog->setAttribute(Qt::WA_DeleteOnClose);
                    currentUserInfoDialog->setCurrentUser(contact);
                    QPoint btnGlobalPos = this->mapToGlobal(QPoint(0,0));
                    currentUserInfoDialog->showAtPos(QPoint(btnGlobalPos.x() + width(), btnGlobalPos.y()));
                    currentUserInfoDialog->setWeChatWidget(this);
                    currentUserInfoDialog->setMediaDialog(mediaDialog);
                }
                else{
                    ClickClosePopup *popup = new ClickClosePopup;

                    QVBoxLayout *contentLayout = new QVBoxLayout(popup);
                    contentLayout->setContentsMargins(1, 1, 1, 1);
                    contentLayout->setSpacing(0);
                    contentLayout->setAlignment(Qt::AlignCenter);

                    UserInfoWidget *userInfoWidget = new UserInfoWidget;
                    userInfoWidget->setWeChatWidget(this);
                    userInfoWidget->setMediaDialog(mediaDialog);
                    userInfoWidget->setSelectedContact(contact);

                    contentLayout->addWidget(userInfoWidget);
                    popup->setFixedWidth(325);
                    popup->setFixedHeight(490);
                    popup->enableClickCloseFeature();
                    popup->showAtPos(QCursor::pos());
                }
    });
}

// 当前登录用户
void WeChatWidget::initCurrentUser()
{
    ui->avatarPushButton->setWeChatWidget(this);
    ui->avatarPushButton->setMediaDialog(mediaDialog);
    connect(contactController, &ContactController::currentUserLoaded, this,
            [this](int reqId, const Contact& contact){
                currentUser = contact;
                userController->setCurrentUser(contact.user);
                messageController->setCurrentUser(1,contact.user);
                ui->avatarPushButton->setContact(currentUser);
            });
    contactController->getCurrentUser();
}

// 联系人列表
void WeChatWidget::initContactList()
{
    // 通讯录
    userInfoWidget = ui->userInfoWidget;
    userInfoWidget->setWeChatWidget(this);
    userInfoWidget->setMediaDialog(mediaDialog);

    contactTreeView = ui->contactTreeView;
    contactItemDelegate = new ContactItemDelegate(contactController->contactTreeModel(), contactTreeView);
    contactTreeView->setItemDelegate(contactItemDelegate);
    contactTreeView->setContactModel(contactController->contactTreeModel());
    // 联系人项选中改变时
    connect(contactTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected, const QItemSelection &deselected){

                if (!selected.isEmpty() && contactTreeView->getSelectedContact().isValid()){
                    m_contact = contactTreeView->getSelectedContact();
                    ui->rightStackedWidget->setCurrentWidget(ui->rightStackedWidgetpage1);
                    ui->rightStackedWidgetpage1->show();
                    userInfoWidget->setSelectedContact(m_contact);

                }else if (!deselected.isEmpty()) {
                    ui->rightStackedWidgetpage1->hide();
                }
            });
    // 菜单信号连接
    connect(contactTreeView, &ContactTreeView::sendMessage, this,
            [this](const Contact &contact){
                on_switchtoMessageInterface(contact);
            });
    connect(contactTreeView, &ContactTreeView::starFriend, this,
            [this](const Contact &contact){
                contactController->setContactStarred(0, contact.userId, !contact.isStarred);
            });
    connect(contactTreeView, &ContactTreeView::removeFriend, this,
            [this](const Contact &contact){
                contactController->deleteContact(0, contact.userId);
            });

    // 删除成功
    connect(contactController, &ContactController::contactDeleted, this,
            [this](int reqId, bool success, const QString& error){
                if(!success) qDebug()<<error;
                ui->rightStackedWidgetpage1->hide();
            });

    // 其他业务情况加载时重新选中加载前的项
    connect(contactController, &ContactController::allContactsLoaded,
            this, [this](int reqId, const QList<Contact>& contacts){
                if (m_contact.isValid()) {
                    ContactTreeModel *m_model = contactController->contactTreeModel();
                    QModelIndex contactIndex = m_model->findContactIndex(m_contact.userId);

                    if (contactIndex.isValid()) {
                        contactTreeView->setCurrentIndex(contactIndex);
                        contactTreeView->scrollTo(contactIndex);
                    }
                }
            });
    contactController->getAllContacts(0);
}

WeChatWidget::~WeChatWidget()
{
    delete ui;
    qApp->removeEventFilter(this);
}


// 绘制边框-------------------------------------------
void WeChatWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(160,160,160),1);
    painter.setPen(pen);
    painter.setBrush(QColor(238,238,238));

    QRect rect = QRect(1,1,width()-2,height()-2);
    painter.drawRoundedRect(rect,m_isMaximized? 0:6,m_isMaximized? 0:6);
}

// 判断鼠标位置是否在窗口边缘------------------------------------------
WeChatWidget::Edge WeChatWidget::getEdge(const QPoint &pos)
{
    // 获取窗口矩形
    QRect rect = this->rect();
    if (pos.x() <= m_borderWidth && pos.y() <= m_borderWidth)
        return TopLeft;

    if (pos.x() >= rect.width() - m_borderWidth && pos.y() <= m_borderWidth)
        return TopRight;

    if (pos.x() <= m_borderWidth && pos.y() >= rect.height() - m_borderWidth)
        return BottomLeft;

    if (pos.x() >= rect.width() - m_borderWidth && pos.y() >= rect.height() - m_borderWidth)
        return BottomRight;

    if (pos.x() <= m_borderWidth)
        return Left;

    if (pos.x() >= rect.width() - m_borderWidth)
        return Right;

    if (pos.y() <= m_borderWidth)
        return Top;

    if (pos.y() >= rect.height() - m_borderWidth)
        return Bottom;

    return None;
}

// 更新鼠标样式------------------------------------------------
void WeChatWidget::updateCursorShape(const QPoint &pos)
{
    Edge edge = getEdge(pos);

    switch (edge) {
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case TopLeft:
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRight:
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

// 处理拉伸逻辑
void WeChatWidget::handleResize(const QPoint &currentGlobalPos)
{
    if (m_currentEdge == None || m_isMaximized) return;

    QRect newGeometry = m_windowGeometry;
    switch (m_currentEdge) {
    case Left:
        newGeometry.setLeft(qMin(currentGlobalPos.x(), newGeometry.right() - minimumWidth()));
        break;
    case Right:
        newGeometry.setRight(qMax(currentGlobalPos.x(), newGeometry.left() + minimumWidth()));
        break;
    case Top:
        newGeometry.setTop(qMin(currentGlobalPos.y(), newGeometry.bottom() - minimumHeight()));
        break;
    case Bottom:
        newGeometry.setBottom(qMax(currentGlobalPos.y(), newGeometry.top() + minimumHeight()));
        break;
    case TopLeft:
        newGeometry.setLeft(qMin(currentGlobalPos.x(), newGeometry.right() - minimumWidth()));
        newGeometry.setTop(qMin(currentGlobalPos.y(), newGeometry.bottom() - minimumHeight()));
        break;
    case TopRight:
        newGeometry.setRight(qMax(currentGlobalPos.x(), newGeometry.left() + minimumWidth()));
        newGeometry.setTop(qMin(currentGlobalPos.y(), newGeometry.bottom() - minimumHeight()));
        break;
    case BottomLeft:
        newGeometry.setLeft(qMin(currentGlobalPos.x(), newGeometry.right() - minimumWidth()));
        newGeometry.setBottom(qMax(currentGlobalPos.y(), newGeometry.top() + minimumHeight()));
        break;
    case BottomRight:
        newGeometry.setRight(qMax(currentGlobalPos.x(), newGeometry.left() + minimumWidth()));
        newGeometry.setBottom(qMax(currentGlobalPos.y(), newGeometry.top() + minimumHeight()));
        break;
    default:
        break;
    }
    setGeometry(newGeometry);
}

// 处理移动逻辑
void WeChatWidget::handleDrag(const QPoint &currentGlobalPos)
{
    if (m_isDraggingMax) {
        // 最大化时拖动逻辑
        on_maxWinButton_clicked();
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();
        int screenWidth = screenGeometry.width();
        int screenHeight = screenGeometry.height();
        double horizontalRatio = static_cast<double>(currentGlobalPos.x()) / screenWidth;
        int mouseXInWindow = static_cast<int>(horizontalRatio * width());
        int mouseYInWindow = m_titleBarHeight / 2;
        int newWindowX = currentGlobalPos.x() - mouseXInWindow;
        int newWindowY = currentGlobalPos.y() - mouseYInWindow;
        newWindowX = qMax(0, qMin(newWindowX, screenWidth - width()));
        newWindowY = qMax(0, qMin(newWindowY, screenHeight - height()));
        move(newWindowX, newWindowY);
        m_dragStartPosition = currentGlobalPos - frameGeometry().topLeft();
        m_isDraggingMax = false;
    } else {
        // 正常拖动
        move(currentGlobalPos - m_dragStartPosition);
    }
}

// 窗口鼠标按下事件
void WeChatWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_currentEdge = getEdge(event->pos());
        // 边缘拉伸初始化
        if (m_currentEdge != None && !m_isMaximized) {
            m_isResizing = true;
            m_windowGeometry = geometry();
            event->accept();
        }
        // 标题栏移动初始化
        else if (event->pos().y() <= m_titleBarHeight) {
            if (m_isMaximized) m_isDraggingMax = true;
            m_dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_isDragging = true;
            event->accept();
        }
    }
    QWidget::mousePressEvent(event);
}

// 窗口鼠标移动事件（调用公共函数处理逻辑）
void WeChatWidget::mouseMoveEvent(QMouseEvent *event)
{
    updateCursorShape(event->pos());
    // 拉伸：调用handleResize
    if (m_isResizing && m_currentEdge != None && !m_isMaximized) {
        handleResize(event->globalPosition().toPoint()); // 传入全局坐标
        event->accept();
    }
    // 移动：调用handleDrag
    else if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        handleDrag(event->globalPosition().toPoint()); // 传入全局坐标
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

// 鼠标释放事件
void WeChatWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDragging = false;
    m_isDraggingMax = false;
    m_isResizing = false;
    m_currentEdge = None;
    setCursor(Qt::ArrowCursor);
    event->accept();
    QWidget::mouseReleaseEvent(event);
}

bool WeChatWidget::eventFilter(QObject *watched, QEvent *event) {
    // 筛选「鼠标移动事件」（QApplication的事件）更新光标
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos = mouseEvent->globalPosition().toPoint();//全局坐标
        if (this->geometry().contains(this->mapFromGlobal(globalPos))) {
            QPoint windowLocalPos = this->mapFromGlobal(globalPos);
            this->updateCursorShape(windowLocalPos);
        }
        else {
            this->setCursor(Qt::ArrowCursor);
        }
    }
    return false;
}

void WeChatWidget::resizeEvent(QResizeEvent *event)
{
    rightStackedWidgetPageSizeChange();
    QWidget::resizeEvent(event);
}

//动态设置发送按钮样式
void WeChatWidget::updateSendButtonStyle(){
    QTextEdit *sendTextEdit = this->findChild<QTextEdit*>("sendTextEdit");
    QPushButton* sendButton = this->findChild<QPushButton*>("sendPushButton");
    QString text = sendTextEdit->toPlainText().trimmed();
    bool isEmpty = text.isEmpty();
    if(isEmpty){
        //文本框为空：应用空状态样式
        sendButton->setProperty("state","empty");
        sendButton->setStyleSheet("QPushButton[state=\"empty\"] { "
                                  "background-color: rgb(220, 220, 220); "
                                  "color: rgb(150, 150, 150); "
                                  "font: 15px \"黑体\"; "
                                  "border-radius: 3px; "
                                  "}");
        sendButton->setEnabled(false);
    }else{
        // 文本框有内容：恢复原样式
        sendButton->setProperty("state", "normal");
        sendButton->setStyleSheet("QPushButton[state=\"normal\"] { "
                                      "background-color: rgb(7, 193, 96); "
                                      "color: rgb(255, 255, 255); "
                                      "font: 15px \"黑体\"; "
                                      "border-radius: 3px; "
                                      "}"
                                      "QPushButton[state=\"normal\"]:hover { "
                                      "background-color: rgb(7, 182, 88); "
                                      "}"
                                      "QPushButton[state=\"normal\"]:pressed { "
                                      "background-color: rgb(6, 178, 83); "
                                      "}");
        sendButton->setEnabled(true);
    }
    sendButton->style()->unpolish(sendButton);
    sendButton->style()->polish(sendButton);
    sendButton->update();
}

void WeChatWidget::on_contactsToolButton_clicked()
{
    QStackedWidget* rightStackedWidget = ui->rightStackedWidget;
    QStackedWidget* leftStackedWidget = ui->leftStackedWidget;
    rightStackedWidget->setCurrentIndex(1);
    leftStackedWidget->setCurrentIndex(1);

    // 如果没有选中联系人，就不显示联系人详细界面。
    if(!contactTreeView->getSelectedContact().isValid())
        ui->rightStackedWidgetpage1->hide();
    contactController->getAllContacts(0);

}

void WeChatWidget::on_collectionToolButton_clicked()
{
    QStackedWidget* rightStackedWidget = ui->rightStackedWidget;
    QStackedWidget* leftStackedWidget = ui->leftStackedWidget;
    rightStackedWidget->setCurrentIndex(2);
    leftStackedWidget->setCurrentIndex(2);
}

void WeChatWidget::on_chatInterfaceToolButton_clicked()
{
    QStackedWidget* rightStackedWidget = ui->rightStackedWidget;
    QStackedWidget* leftStackedWidget = ui->leftStackedWidget;
    rightStackedWidget->setCurrentIndex(0);
    leftStackedWidget->setCurrentIndex(0);

    if(!currentConversation.isValid()) ui->rightStackedWidgetPage0->hide();
    conversationController->loadConversations(0);
}

void WeChatWidget::on_rightDialogToolButton_clicked()
{
    if(rightPopover){
        //隐藏动画：滑回主窗口右侧外部
        QPropertyAnimation *anim = new QPropertyAnimation(rightPopover,"pos");
        anim->setDuration(300);
        QPoint startPos = rightPopover->pos();
        QPoint endPos(startPos.x() + 254, startPos.y());
        anim->setStartValue(startPos);
        anim->setEndValue(endPos);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        connect(anim,&QPropertyAnimation::finished, rightPopover,&QWidget::close);
    }else{
        rightPopover = new RightPopover(ui->rightStackedWidgetPage0);
        connect(rightPopover,&RightPopover::cloesDialog,this, &WeChatWidget::on_rightDialogToolButton_clicked);
        rightPopover->setAttribute(Qt::WA_DeleteOnClose);

        rightPopover->setContact(contactController->getContactFromModel(currentConversation.userId));
        rightPopover->setMediaDialog(mediaDialog);
        rightPopover->setWeChatWidget(this);

        QCheckBox *isTopCheckBox = rightPopover->findChild<QCheckBox*>("isTopCheckBox");
        isTopCheckBox->setChecked(currentConversation.isTop);
        connect(isTopCheckBox, &QCheckBox::toggled, this,
            [this](){
            conversationController->handleToggleTop(currentConversation.conversationId);
        });

        //先在窗口右外侧看不到的地方显示
        int startX = ui->rightStackedWidgetPage0->width();
        int startY = ui->messageSplitter->pos().y();
        rightPopover->setGeometry(startX,startY,254,this->height());
        rightPopover->show();

        //再从右侧滑动出来
        QPropertyAnimation *anim = new QPropertyAnimation(rightPopover,"pos");
        anim->setDuration(300);
        QPoint startPos(startX,startY);
        QPoint endPos(startX-254,startY);
        anim->setStartValue(startPos);
        anim->setEndValue(endPos);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void WeChatWidget::rightStackedWidgetPageSizeChange()
{
    if(rightPopover){
        int rpfX = ui->rightStackedWidgetPage0->width() - 254;
        int rpfY = rightPopover->pos().y();
        int rpfWidth = 254;
        int rpfHeight = ui->rightStackedWidgetPage0->height();
        rightPopover->setGeometry(rpfX,rpfY,rpfWidth,rpfHeight);
    }
}

void WeChatWidget::on_addToolButton_clicked()
{
    //显示对话框
    if(!addDialog){
        addDialog = new AddDialog(contactController, this);
        addDialog->setAttribute(Qt::WA_DeleteOnClose);
        addDialog->showAtPos(QCursor::pos());
    }
}

void WeChatWidget::on_moreToolButton_clicked()
{
    if(!moreDialog){
        moreDialog = new MoreDialog();
        moreDialog->setAttribute(Qt::WA_DeleteOnClose);
        QToolButton* btn = this->findChild<QToolButton*>("moreToolButton");
        QPoint buttonPos = btn->mapToGlobal(QPoint(0,0));
        int x = buttonPos.x() + btn->width();
        int y = buttonPos.y()-200 + btn->height();
        moreDialog->showAtPos(QPoint(x,y));
    }
}

void WeChatWidget::on_floatingToolButton_clicked()
{
    if(!floatingDialog){
        floatingDialog = new FloatingDialog();
        floatingDialog->setAttribute(Qt::WA_DeleteOnClose);
        QToolButton *btn = this->findChild<QToolButton*>("floatingToolButton");
        QPoint buttonPos = btn->mapToGlobal(QPoint(0,0));
        int x = buttonPos.x() + btn->width();
        int y = buttonPos.y()-395 + btn->height();
        floatingDialog->showAtPos(QPoint(x,y));
    }
}

void WeChatWidget::on_avatarPushButton_clicked()
{
    contactController->getCurrentUser();// 点击头像按钮主动加载当前用户；
}

void WeChatWidget::on_closeButton_clicked()
{
    close();
}

// 窗口最大化和还原
void WeChatWidget::on_maxWinButton_clicked()
{
    m_isMaximized = !m_isMaximized;
    if (m_isMaximized) {
        showMaximized();
        QToolButton* toolBtn = this->findChild<QToolButton*>("maxWinButton");
        if (toolBtn) {
            toolBtn->setIcon(QIcon(":/a/icons/还原.svg"));
        }
    } else {
        showNormal();
        QToolButton* toolBtn = this->findChild<QToolButton*>("maxWinButton");
        if (toolBtn) {
            toolBtn->setIcon(QIcon(":/a/icons/窗口最大化.svg"));
        }
    }
}

void WeChatWidget::on_minWinButton_clicked()
{
    showMinimized();
}

void WeChatWidget::on_pinButton_clicked()
{
    // 切换置顶状态
    m_isOnTop = !m_isOnTop;
    if (m_isOnTop) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        ui->pinButton->setToolTip("取消置顶");
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
        ui->pinButton->setToolTip("置顶");
    }
    show();
}

void WeChatWidget::on_selectFileButton_clicked()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "选择文件",
        QDir::homePath(),
        "所有文件 (*.*);;图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
        );

    if (!filePaths.isEmpty()) {
        ui->sendTextEdit->insertFiles(filePaths);
    }
}

void WeChatWidget::on_sendPushButton_clicked()
{
    QList<FileItem> fileItems = ui->sendTextEdit->getFileItems();
    QString text = ui->sendTextEdit->toPlainText().trimmed();
    text.remove(QChar::ObjectReplacementCharacter);

    QStringList imgMsg;
    QStringList fileMsg;
    QStringList vidMsg;
    for (const FileItem &item : std::as_const(fileItems)) {
        if(item.isImage){
            imgMsg << item.filePath;
        }
        else if(item.isVideo){
            vidMsg << item.filePath;
        }
        else{
            fileMsg << item.filePath;
        }
    }

    if(!vidMsg.isEmpty()){
        messageController->preprocessVideoBeforeSend(vidMsg);
    }

    if(!fileMsg.isEmpty()){
        messageController->preprocessFileBeforeSend(fileMsg);
    }

    if(!imgMsg.isEmpty()){
        messageController->preprocessImageBeforeSend(imgMsg);
    }

    if(!text.isEmpty()){
        messageController->sendTextMessage(text);
    }

    // 发送完成后可以清空内容
    ui->sendTextEdit->clearContent();
}

void WeChatWidget::on_recordVoiceButton_clicked()
{
    VoiceRecordDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        QString wavPath = dialog.getRecordedFilePath();
        int duration = dialog.getAudioDuration();
        messageController->sendVoiceMessage(wavPath, duration);

    } else {
        qDebug() << "用户取消了录音";
    }
}

void WeChatWidget::on_switchtoMessageInterface(Contact contact)
{
    ui->rightStackedWidget->setCurrentIndex(0);
    ui->leftStackedWidget->setCurrentIndex(0);

    ChatListModel *m_model = conversationController->chatListModel();
    QModelIndex index = m_model->getConversationIndexByContactId(contact.userId);

    if (index.isValid()) {
        chatListView->setCurrentIndex(index);
        chatListView->scrollTo(index);
    }else{
        conversationController->createSingleChat(m_contact);
    }
}

void WeChatWidget::on_momentButton_clicked()
{
    if(!momentMainWidget){
        momentMainWidget = new MomentMainWidget(localMomentController);
    }
    momentMainWidget->close();
    momentMainWidget->setWeChatWidget(this);
    momentMainWidget->setMediaDialog(mediaDialog);
    momentMainWidget->setContactController(contactController);
    momentMainWidget->setCurrentUser(currentUser);
    momentMainWidget->setLocalMomentController(localMomentController);

    // 计算屏幕中心并移动窗口
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = screenGeometry.center().x() - momentMainWidget->width() / 2;
    int y = screenGeometry.center().y() - momentMainWidget->height() / 2;
    momentMainWidget->move(x, y);

    momentMainWidget->show();
}
