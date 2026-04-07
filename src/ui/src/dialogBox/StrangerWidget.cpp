#include "StrangerWidget.h"
#include "ThumbnailResourceManager.h"
#include "ui_StrangerWidget.h"
#include <QRandomGenerator>

StrangerWidget::StrangerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StrangerWidget)
{
    ui->setupUi(this);
    ThumbnailResourceManager *mediaManager = ThumbnailResourceManager::instance();
    connect(mediaManager, &ThumbnailResourceManager::mediaLoaded,
            this, [this](){
        ThumbnailResourceManager *mediaManager = ThumbnailResourceManager::instance();
        QPixmap avatar = mediaManager->getThumbnail(
        m_user.avatar_local_pathValue(),
        QSize(500, 500),
        MediaType::Avatar,
        50,
        "",
        m_user.nicknameValue()
        );
    ui->avatarLabel->setPixmap(avatar);});
}

StrangerWidget::~StrangerWidget()
{
    delete ui;
}

void StrangerWidget::on_addButton_clicked()
{
    if(m_user.isValid()){
        emit addFriend(m_user);
    }
}

void StrangerWidget::setMediaDialog(MediaDialog* mediaDialog)
{
    m_mediaDialog = mediaDialog;
    ui->avatarLabel->setMediaDialog(mediaDialog);
}

void StrangerWidget::updateUser(const User& user){
    m_user = user;
    qDebug()<<"m_user.user_idValue()"<<m_user.user_idValue();
    ThumbnailResourceManager *mediaManager = ThumbnailResourceManager::instance();
    QPixmap avatar = mediaManager->getThumbnail(
        user.avatar_local_pathValue(),
        QSize(500, 500),
        MediaType::Avatar,
        50,
        "",
        user.nicknameValue()
        );
    ui->avatarLabel->setPixmap(avatar);
    ui->nickname->setText(user.nicknameValue());
    ui->region->setText(user.regionValue());
}
