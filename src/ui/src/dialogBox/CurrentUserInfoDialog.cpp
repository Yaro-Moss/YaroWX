#include "CurrentUserInfoDialog.h"
#include "WeChatWidget.h"
#include "ui_CurrentUserInfoDialog.h"
#include <QPainter>
#include <QPainterPath>
#include "AvatarButton.h"

CurrentUserInfoDialog::CurrentUserInfoDialog(QWidget *parent)
    : ClickClosePopup(parent)
    , ui(new Ui::CurrentUserInfoDialog)
    , m_weChatWidget(nullptr)
    , m_mediaDialog(nullptr)
{
    ui->setupUi(this);
    enableClickCloseFeature();
    avatarLabel = ui->avatarLabel;
    account = ui->account;
    region = ui->region;
    nickname = ui->nickname;
    connect(avatarLabel, &ImgLabel::labelClicked, this, &CurrentUserInfoDialog::close);
}

CurrentUserInfoDialog::~CurrentUserInfoDialog()
{
    delete ui;
}

void CurrentUserInfoDialog::setCurrentUser(const Contact &user)
{
    currentUser = user;
    QPixmap avatar = QPixmap(user.user.avatar_local_pathValue());
    if(avatar.isNull())
        avatar = AvatarButton::generateDefaultAvatar(QSize(500,500), currentUser.user.nicknameValue());
    avatarLabel ->setPixmap(avatar);
    account->setText(currentUser.user.accountValue());
    nickname->setText(currentUser.user.nicknameValue());
    region->setText(currentUser.user.regionValue());
}

void CurrentUserInfoDialog::setMediaDialog(MediaDialog* mediaDialog){
    m_mediaDialog = mediaDialog;
    avatarLabel->setMediaDialog(m_mediaDialog);
}

void CurrentUserInfoDialog::setPixmap(const QPixmap &pixmap)
{
    avatarLabel ->setPixmap(pixmap);
}

void CurrentUserInfoDialog::on_switchMessageInterfaceToolButton_clicked()
{
    if(!m_weChatWidget){
        qDebug()<<"[CurrentUserInfoDialog::on_switchMessageInterfaceToolButton_clicked]控指针 m_weChatWidget";
        return;
    }
    m_weChatWidget->on_switchtoMessageInterface(currentUser);
    m_weChatWidget->close();
    m_weChatWidget->show();
    close();
}

