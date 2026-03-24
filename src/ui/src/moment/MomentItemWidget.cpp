#include "MomentItemWidget.h"
#include "CommentItemWidget.h"
#include "ContactController.h"
#include "FlowLayout.h"
#include "FormatTime.h"
#include "ImgLabel.h"
#include "LocalMomentController.h"
#include "ui_MomentItemWidget.h"
#include <qmenu.h>
#include <QWidgetAction>

MomentItemWidget::MomentItemWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MomentItemWidget)
    , m_weChatWidget(nullptr)
    , m_mediaDialog(nullptr)
    , m_contactController(nullptr)
    , m_localMomentController(nullptr)
{
    ui->setupUi(this);
    commentInputWidget = new CommentInputWidget(ui->interactionWidget);
    ui->interactionWidget->layout()->addWidget(commentInputWidget);
    commentInputWidget->hide();
    connect(commentInputWidget, &CommentInputWidget::commentSent, this, &MomentItemWidget::onCommentSent);

    Margins mediaMargins = Margins(5,7,0,0);
    m_mediaFlowLayout = new FlowLayout(nullptr, mediaMargins, 3, 3);
    ui->mediaWidget->setLayout(m_mediaFlowLayout);

    Margins likeMargins = Margins(14,3,0,7);
    m_likeFlowLayout = new FlowLayout(nullptr, likeMargins, 6, 6);
    ui->likeWidget->setLayout(m_likeFlowLayout);
    m_likeFlowLayout->addWidget(ui->heartLabel);

    ui->commentWidget->layout()->setAlignment(Qt::AlignTop);
    setStyleSheet("background-color: rgb(252, 252, 252);");

    createInteractMenu();
}

void MomentItemWidget::setCurrentUser(const Contact &currentUser)
{
    m_currentUser = currentUser;
}


MomentItemWidget::~MomentItemWidget()
{
    delete ui;
}

void MomentItemWidget::createInteractMenu()
{
    // 创建自定义菜单（无边框、透明背景）
    m_interactMenu = new QMenu(this);
    m_interactMenu->setStyleSheet(R"(
        QMenu {
            background-color: transparent;
            border: none;
            padding: 0px;
        }
        QMenu::item {
            padding: 0px;
            margin: 0px;
        }
    )");

    // 2. 创建水平布局的Widget
    QWidget *menuWidget = new QWidget(m_interactMenu);
    QHBoxLayout *hLayout = new QHBoxLayout(menuWidget);
    hLayout->setSpacing(0);    // 按钮间距0
    hLayout->setContentsMargins(0, 0, 0, 0); // 布局内边距0

    // 创建点赞按钮（左上下圆角5px）
    likeBtn = new QPushButton(QIcon(":/a/icons/爱心.svg"), "点赞", menuWidget);
    likeBtn->setObjectName("likeButton");
    likeBtn->setCursor(Qt::PointingHandCursor);

    // 创建评论按钮（右上下圆角5px）
    commentBtn = new QPushButton(QIcon(":/a/icons/评论.svg"), "评论", menuWidget);
    commentBtn->setObjectName("commentButton");
    commentBtn->setCursor(Qt::PointingHandCursor);

    // 按钮样式表
    QString btnStyle = R"(
        QPushButton {
            background-color: rgb(77, 81, 84);
            color: white;
            border: none;
            padding: 8px 16px;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: rgb(86, 89, 92);
        }
        #likeButton {
            border-top-left-radius: 5px;
            border-bottom-left-radius: 5px;
        }
        #commentButton {
            border-top-right-radius: 5px;
            border-bottom-right-radius: 5px;
        }
    )";
    likeBtn->setStyleSheet(btnStyle);
    commentBtn->setStyleSheet(btnStyle);

    // 绑定点击事件
    connect(likeBtn, &QPushButton::clicked, this, &MomentItemWidget::onLikeActionTriggered);
    connect(commentBtn, &QPushButton::clicked, this, &MomentItemWidget::onCommentActionTriggered);

    // 添加按钮到布局
    hLayout->addWidget(likeBtn);
    hLayout->addWidget(commentBtn);
    menuWidget->setLayout(hLayout);

    // 包装成WidgetAction并添加到菜单
    QWidgetAction *menuAction = new QWidgetAction(m_interactMenu);
    menuAction->setDefaultWidget(menuWidget);
    m_interactMenu->addAction(menuAction);
}

// 通用的布局清空辅助函数
void MomentItemWidget::clearLayout(QLayout *layout)
{
    if (!layout) return; // 空布局直接返回，避免空指针
    QLayoutItem *item = nullptr;
    // 遍历布局中所有项，逐个删除
    while ((item = layout->takeAt(0)) != nullptr)
    {
        // 删除布局项对应的控件（核心：释放旧组件内存）
        if (QWidget *widget = item->widget())
        {
            widget->deleteLater(); // 安全删除控件，避免界面卡顿
        }
        // 递归清空子布局（如果有嵌套布局）
        if (QLayout *childLayout = item->layout())
        {
            clearLayout(childLayout);
        }
        delete item; // 删除布局项本身
    }
}

void MomentItemWidget::setLocalMoment(const LocalMoment &localMoment)
{
    m_localMoment = localMoment;
    ui->avatarPushButton->setMediaDialog(m_mediaDialog);
    ui->avatarPushButton->setWeChatWidget(m_weChatWidget);

    ui->userNameButton->setMode(AvatarButton::NameMode);
    ui->userNameButton->setMediaDialog(m_mediaDialog);
    ui->userNameButton->setWeChatWidget(m_weChatWidget);

    if(m_contactController){
        ui->avatarPushButton->setContact(m_contactController->getContactFromModel(m_localMoment.userId));
        ui->userNameButton->setContact(m_contactController->getContactFromModel(m_localMoment.userId));
    }else{
        qDebug()<<"[MomentItemWidget::setLocalMoment] 空指针m_contactController";
    }

    if(localMoment.content.isEmpty()){
        ui->textEdit->hide();
    }else{
        ui->textEdit->setText(localMoment.content);
        ui->textEdit->show();
    }

    clearLayout(m_mediaFlowLayout);
    ui->mediaWidget->show();
    if(!localMoment.videoLocalPath.isEmpty()){
        ImgLabel *lab = new ImgLabel(ui->mediaWidget);
        lab->setFixedSize(150,200);
        lab->setVideoPath(localMoment.videoLocalPath);
        lab->setMediaDialog(m_mediaDialog);
        m_mediaFlowLayout->addWidget(lab);
    }
    else if(!localMoment.images.isEmpty()){
        for(const MomentImageInfo& img : localMoment.images){
            ImgLabel *imgLabel = new ImgLabel(ui->mediaWidget, 0);
            imgLabel->setFixedSize(150, 150);
            imgLabel->setPixmap(QPixmap(img.localPath));
            imgLabel->setMediaDialog(m_mediaDialog);
            m_mediaFlowLayout->addWidget(imgLabel);
        }
    }else{
        ui->mediaWidget->hide();
    }

    // 时间
    ui->dateLabel->setText(FormatTime(localMoment.createTime));

    // ---互动信息---
    ui->interactionWidget->show();
    bool hide = true;
    clearLayout(m_likeFlowLayout);
    ui->likeWidget->show();
    QVector<MomentLikeInfo> likes = localMoment.interact.likes;
    if(likes.isEmpty()){
        ui->likeWidget->hide();
        likeBtn->setText("点赞");
    }else{
        hide = false;
        QLabel *lab = new QLabel(ui->likeWidget);
        lab->setFixedSize(35, 35);
        lab->setAlignment(Qt::AlignCenter);
        QPixmap pixmap(":/a/icons/爱心.svg");
        QPixmap scaledPixmap = pixmap.scaled(25, 25,
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        lab->setPixmap(scaledPixmap);
        lab->setScaledContents(false);

        likeBtn->setText("点赞");
        m_likeFlowLayout->addWidget(lab);
        for(const MomentLikeInfo &like: likes){
            AvatarButton *likeAvatarButton = new AvatarButton(ui->likeWidget);
            likeAvatarButton->setMediaDialog(m_mediaDialog);
            likeAvatarButton->setWeChatWidget(m_weChatWidget);
            likeAvatarButton->setFixedSize(37,37);

            if(m_contactController){
                likeAvatarButton->setContact(m_contactController->getContactFromModel(like.userId));
            }else{
                qDebug()<<"[MomentItemWidget::setLocalMoment] 空指针m_contactController";
            }
            m_likeFlowLayout->addWidget(likeAvatarButton);

            if(like.userId == m_localMomentController->currentLoginUserId()){
                likeBtn->setText("取消");
                break;
            }
        }
    }

    // 评论
    QLayout *commentLayout = ui->commentWidget->layout();
    clearLayout(commentLayout);
    ui->commentWidget->show();
    QVector<MomentCommentInfo> comments = localMoment.interact.comments;
    if(comments.isEmpty()){
        ui->commentWidget->hide();
    }else{
        hide = false;
        for (int i = 0; i < comments.size(); i++) {
            CommentItemWidget *commentItem = new CommentItemWidget(ui->commentWidget);
            commentItem->setWeChatWidget(m_weChatWidget);
            commentItem->setContactController(m_contactController);
            commentItem->setMediaDialog(m_mediaDialog);
            if(i==0)commentItem->setIcon();
            commentItem->setMomentCommentInfo(comments[i]);
            commentLayout->addWidget(commentItem);
        }
    }

    if(hide) ui->interactionWidget->hide();

}

void MomentItemWidget::on_rightDialogToolButton_clicked()
{
    QPoint btnGlobalPos = ui->rightDialogToolButton->mapToGlobal(QPoint(0, 0));

    int menuWidth = m_interactMenu->sizeHint().width();
    int targetX = btnGlobalPos.x() - menuWidth;
    int targetY = btnGlobalPos.y() + (ui->rightDialogToolButton->height() - m_interactMenu->sizeHint().height()) / 2; // 垂直居中

    QPoint menuPos(targetX, targetY);
    // 弹出菜单
    m_interactMenu->exec(menuPos);
}

// 点赞动作的业务逻辑
void MomentItemWidget::onLikeActionTriggered()
{
    if(!m_localMomentController){
        qDebug()<<"[MomentItemWidget::onLikeActionTriggered] 空指针m_localMomentController";
        return;
    }
    QVector<MomentLikeInfo> likes = m_localMoment.interact.likes;
    // 获取旧的点赞状态
    bool isLike = false;
    for(const MomentLikeInfo &like: likes){
        if(like.userId == m_localMomentController->currentLoginUserId()){
            isLike =true;
            break;
        }
    }
    // 修改点赞状态
    m_localMomentController->likeMoment(m_localMoment.momentId, !isLike);
    m_interactMenu->hide();
}

// 评论动作的业务逻辑（示例）
void MomentItemWidget::onCommentActionTriggered()
{
    m_interactMenu->hide();
    ui->interactionWidget->show();
    commentInputWidget->show();
}

void MomentItemWidget::onCommentSent(const QString& commentText, const QString& img)
{
    MomentCommentInfo momentCommentInfo;
    momentCommentInfo.commentId = QDateTime::currentMSecsSinceEpoch();
    momentCommentInfo.createTime = QDateTime::currentSecsSinceEpoch();
    momentCommentInfo.avatarLocalPath = m_currentUser.user.avatar_local_pathValue();
    momentCommentInfo.replyUserId = m_localMoment.userId;
    momentCommentInfo.userId = m_currentUser.user_idValue();
    momentCommentInfo.username = m_currentUser.remark_nameValue();
    momentCommentInfo.content = commentText;
    momentCommentInfo.image.localPath = img;

    m_localMomentController->addComment(m_localMoment.momentId, momentCommentInfo);
    if(!ui->likeWidget->isVisible()&&!ui->commentWidget->isVisible())
        ui->interactionWidget->hide();
}
