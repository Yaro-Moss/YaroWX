#include "UserInfoWidget.h"
#include "ContactController.h"
#include "WeChatWidget.h"
#include "AvatarButton.h"
#include "ui_NewFriend.h"
#include "ui_UserInfoWidget.h"
#include <QVBoxLayout>
#include <QFile>
#include "AddFriendRequestDialog.h"

UserInfoWidget::UserInfoWidget(QWidget *parent)
    : QWidget(parent)
    , m_stackedWidget(new QStackedWidget(this))
    , m_userInfoPage(new QWidget)
    , m_newFriendPage(new QWidget)
    , m_userInfoUi(new Ui::UserInfoWidget)
    , m_newFriendUi(new Ui::NewFriend)
    , m_weChatWidget(nullptr)
    , m_mediaDialog(nullptr)
{
    // 1. 设置用户信息页面（原有界面）
    m_userInfoUi->setupUi(m_userInfoPage);
    avatarLabel        = m_userInfoUi->avatarLabel;
    remarkNameLable    = m_userInfoUi->remarkNameLable;
    nicknameLable      = m_userInfoUi->nicknameLable;
    accountLable       = m_userInfoUi->accountLable;
    regionLable        = m_userInfoUi->regionLable;
    remarkNameButton   = m_userInfoUi->remarkNameButton;
    tagButton          = m_userInfoUi->tagButton;
    signatureLabel     = m_userInfoUi->signatureLabel;
    mutualgroupLable   = m_userInfoUi->mutualgroupLable;
    sourceLabel        = m_userInfoUi->sourceLabel;

    // 2. 设置新朋友页面（NewFriend.ui）
    m_newFriendUi->setupUi(m_newFriendPage);

    // 3. 将两个页面加入 QStackedWidget
    m_stackedWidget->addWidget(m_userInfoPage);
    m_stackedWidget->addWidget(m_newFriendPage);

    // 4. 设置主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_stackedWidget);
    setLayout(mainLayout);

    // 默认显示用户信息页
    m_stackedWidget->setCurrentIndex(0);

    connect(m_newFriendUi->verifyButton, &QPushButton::clicked, this, &UserInfoWidget::onVerifyButton);
    connect(m_userInfoUi->switchMessageInterfaceToolButton, &QPushButton::clicked, this, &UserInfoWidget::onSwitchMessageInterfaceToolButton);
}

UserInfoWidget::~UserInfoWidget()
{
    delete m_userInfoUi;
    delete m_newFriendUi;
    // m_stackedWidget, m_userInfoPage, m_newFriendPage 会由 Qt 对象树自动删除
}

void UserInfoWidget::setMediaDialog(MediaDialog* mediaDialog)
{
    m_mediaDialog = mediaDialog;
    // 原有头像控件需要设置媒体对话框（用于点击放大）
    avatarLabel->setMediaDialog(m_mediaDialog);
    m_newFriendUi->avatarLabel->setMediaDialog(m_mediaDialog);
}

void UserInfoWidget::setSelectedContact(const Contact &contact)
{
    switchToUserInfoPage();
    m_contact = contact;

    // ========== 更新用户信息页面 ==========
    QPixmap avatar(m_contact.user.avatar_local_pathValue());
    if (avatar.isNull())
        avatar = AvatarButton::generateDefaultAvatar(QSize(500, 500), m_contact.user.nicknameValue());
    avatarLabel->setPixmap(avatar);

    remarkNameLable->setText(m_contact.remark_nameValue());
    nicknameLable->setText(m_contact.user.nicknameValue());
    accountLable->setText(m_contact.user.accountValue());
    regionLable->setText(m_contact.user.regionValue());
    remarkNameButton->setText(m_contact.remark_nameValue());
    tagButton->setText(m_contact.getTagsString());
    signatureLabel->setText(m_contact.user.signatureValue());
    mutualgroupLable->setText("9");  // 暂未实现
    sourceLabel->setText(m_contact.sourceValue());

}

void UserInfoWidget::switchToUserInfoPage()
{
    m_stackedWidget->setCurrentIndex(0);
}

void UserInfoWidget::switchToNewFriendPage()
{
    m_stackedWidget->setCurrentIndex(1);
}

void UserInfoWidget::onSwitchMessageInterfaceToolButton()
{
    if (!m_weChatWidget) return;
    m_weChatWidget->on_switchtoMessageInterface(m_contact);
    m_weChatWidget->close();
    m_weChatWidget->show();
}

void UserInfoWidget::setNewFriend(const FriendRequest &friendRequest)
{
    // 1. 切换到新朋友页面
    switchToNewFriendPage();

    // 2. 获取新朋友页面上的控件指针（为了方便，可以临时获取）
    ImgLabel *avatar = m_newFriendUi->avatarLabel;
    QLabel *remarkName = m_newFriendUi->remarkNameLable;   // 大标题（显示昵称）
    QLabel *nickname = m_newFriendUi->nicknameLable;       // 昵称行（带“昵称：”标签旁边）
    QLabel *account = m_newFriendUi->accountLable;         // 账号
    QLabel *source = m_newFriendUi->sourceLabel;           // 来源
    QTextEdit *messageEdit = m_newFriendUi->textEdit;      // 验证消息输入框

    // 3. 设置头像
    QString avatarPath = friendRequest.from_avatarValue();
    QPixmap avatarPix;
    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        avatarPix.load(avatarPath);
    }
    if (avatarPix.isNull()) {
        // 生成默认头像，使用申请人昵称
        QString nicknameForAvatar = friendRequest.from_nicknameValue();
        avatarPix = AvatarButton::generateDefaultAvatar(QSize(60, 60), nicknameForAvatar);
    }
    avatar->setPixmap(avatarPix);

    // 4. 设置昵称（大标题）
    QString displayName = friendRequest.from_nicknameValue();
    remarkName->setText(displayName);

    // 5. 设置昵称行（可选，如果已经有“昵称：”标签，这里只显示昵称）
    nickname->setText(displayName);

    // 6. 设置账号
    account->setText(friendRequest.from_accountValue());

    // 7. 设置来源
    source->setText(tr("朋友验证"));

    // 8. 设置验证消息（附言）
    QString message = friendRequest.messageValue();
    if (message.isEmpty()) {
        message = tr("我是 %1").arg(displayName);
    }else{
        message = displayName+": "+message;
    }
    messageEdit->setPlainText(message);

    // 9. 可选：保存当前处理的 friendRequest 到成员变量，供后续验证使用
    m_currentFriendRequest = friendRequest;

}


void UserInfoWidget::onVerifyButton()
{
    // 处理“前往验证”按钮点击：同意好友请求
    if (!m_currentFriendRequest.isValid()) {
        qDebug() << "No valid friend request to verify.";
        return;
    }
    // 自动释放资源WA_DeleteOnClose
    AddFriendRequestDialog *addFriendRequestDialog = new AddFriendRequestDialog(m_contactController);
    addFriendRequestDialog->setRequestMode();
    addFriendRequestDialog->setRequestId(m_currentFriendRequest.idValue());
    addFriendRequestDialog->show();
}




