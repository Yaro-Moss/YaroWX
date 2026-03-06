#include "StrangerWidget.h"
#include "ui_StrangerWidget.h"
#include <QRandomGenerator>

StrangerWidget::StrangerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StrangerWidget)
{
    ui->setupUi(this);
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

void StrangerWidget::updateUser(const User& user){
    m_user = user;
    QString avatar = QStringList({":/a/image/.jpg", ":/a/image/avatar.jpg", ":/a/image/fm.jpg", ":/a/image/二维码.png"}).at(QRandomGenerator::global()->bounded(4));

    ui->avatarLabel->setPixmap(QPixmap(avatar));
    ui->nickname->setText(user.nickname);
    ui->region->setText(user.region);
}
