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
    ui->avatarLabel->setPixmap(QPixmap(user.avatarValue()));
    ui->nickname->setText(user.nicknameValue());
    ui->region->setText(user.regionValue());
}
