#include "UserInfoWidget.h"
#include "WeChatWidget.h"
#include "AvatarButton.h"
#include "ui_UserInfoWidget.h"

UserInfoWidget::UserInfoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UserInfoWidget)
    , m_weChatWidget(nullptr)
    , m_mediaDialog(nullptr)
{
    ui->setupUi(this);

    avatarLabel = ui->avatarLabel;
    remarkNameLable = ui->remarkNameLable;
    nicknameLable = ui->nicknameLable;
    accountLable = ui->accountLable;
    regionLable = ui->regionLable;
    remarkNameButton = ui->remarkNameButton;
    tagButton = ui->tagButton;
    signatureLabel = ui->signatureLabel;
    mutualgroupLable = ui->mutualgroupLable;
    sourceLabel = ui->sourceLabel;
}

UserInfoWidget::~UserInfoWidget()
{
    delete ui;
}

void UserInfoWidget::setMediaDialog(MediaDialog* mediaDialog){
    m_mediaDialog = mediaDialog;
    avatarLabel->setMediaDialog(m_mediaDialog);
}

void UserInfoWidget::setSelectedContact(const Contact &contact)
{
    m_contact = contact;

    QPixmap avatar (m_contact.user.avatarLocalPath);
    if(avatar.isNull())
        avatar = AvatarButton::generateDefaultAvatar(QSize(500, 500), m_contact.user.nickname);
    avatarLabel->setPixmap(avatar);

    remarkNameLable->setText(m_contact.remarkName);
    nicknameLable->setText(m_contact.user.nickname);
    accountLable->setText(m_contact.user.account);
    regionLable->setText(m_contact.user.region);
    remarkNameButton->setText(m_contact.remarkName);
    tagButton->setText(m_contact.getTagsString());
    signatureLabel->setText(m_contact.user.signature);
    mutualgroupLable->setText("9");// 忘记实现了，随便写个数字，有空再搞。
    sourceLabel->setText(m_contact.source);
}

void UserInfoWidget::on_switchMessageInterfaceToolButton_clicked()
{
    if(!m_weChatWidget)return;
    m_weChatWidget->on_switchtoMessageInterface(m_contact);
    m_weChatWidget->close();
    m_weChatWidget->show();
}

