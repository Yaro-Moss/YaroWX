#include "CommentItemWidget.h"
#include "ContactController.h"
#include "ui_CommentItemWidget.h"
CommentItemWidget::CommentItemWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CommentItemWidget)
{
    ui->setupUi(this);
}

CommentItemWidget::~CommentItemWidget()
{
    delete ui;
}

void CommentItemWidget::setIcon(){
    ui->commentIconLabel->setPixmap(QPixmap(":/a/icons/评论.svg"));
}

void CommentItemWidget::on_commentAvatarButton_clicked()
{

}


void CommentItemWidget::on_userNameButton_clicked()
{

}


void CommentItemWidget::setMomentCommentInfo(const MomentCommentInfo& momentCommentInfo)
{
    m_momentCommentInfo = momentCommentInfo;

    ui->commentAvatarButton->setContact(m_contactController->getContactFromModel(m_momentCommentInfo.userId));
    ui->commentAvatarButton->setMediaDialog(m_mediaDialog);
    ui->commentAvatarButton->setWeChatWidget(m_weChatWidget);

    if(momentCommentInfo.content.isEmpty()){
        ui->commentText->hide();
    }
    ui->commentText->setText(momentCommentInfo.content);

    ui->userNameButton->setText(momentCommentInfo.username);
    ui->userNameButton->setMode(AvatarButton::NameMode);
    ui->userNameButton->setNameModeFont(QFont("微软雅黑", 12));
    ui->userNameButton->setContact(m_contactController->getContactFromModel(m_momentCommentInfo.userId));
    ui->userNameButton->setMediaDialog(m_mediaDialog);
    ui->userNameButton->setWeChatWidget(m_weChatWidget);

    if(momentCommentInfo.image.localPath.isEmpty()){
        ui->imgLabel->hide();
    }else{
        ui->imgLabel->setPixmap(QPixmap(momentCommentInfo.image.localPath));
        ui->imgLabel->setRadius(0);
        ui->imgLabel->setMediaDialog(m_mediaDialog);
    }
}

