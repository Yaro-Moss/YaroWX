#ifndef USERINFOWIDGET_H
#define USERINFOWIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <qpushbutton.h>
#include "Contact.h"
#include "FriendRequest.h"

// 前置声明 UI 类
namespace Ui {
class UserInfoWidget;
class NewFriend;
}

class MediaDialog;
class WeChatWidget;
class ImgLabel;
class ContactController;

class UserInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UserInfoWidget(QWidget *parent = nullptr);
    ~UserInfoWidget();

    void setSelectedContact(const Contact &contact);
    void setWeChatWidget(WeChatWidget* weChatWidget){m_weChatWidget = weChatWidget;}
    void setMediaDialog(MediaDialog* mediaDialog);
    void setController(ContactController*contactController){
        m_contactController = contactController;}
    void extracted(QString &avatarPath, QPixmap &avatarPix);
    void setNewFriend(const FriendRequest &friendRequest);

    // 新增界面切换方法
    void switchToUserInfoPage();
    void switchToNewFriendPage();

private slots:
    void onSwitchMessageInterfaceToolButton();  // 原槽函数
    void onVerifyButton();// 处理验证按钮点击


private:
    // 容器和页面
    QStackedWidget *m_stackedWidget;
    QWidget *m_userInfoPage;
    QWidget *m_newFriendPage;

    // 两个 UI 对象
    Ui::UserInfoWidget *m_userInfoUi;
    Ui::NewFriend     *m_newFriendUi;

    // 其他原有成员
    WeChatWidget *m_weChatWidget;
    MediaDialog  *m_mediaDialog;
    Contact       m_contact;

    // 原界面上的控件指针（现在从 m_userInfoUi 获取）
    ImgLabel *avatarLabel;
    QLabel *remarkNameLable;
    QLabel *nicknameLable;
    QLabel *accountLable;
    QLabel *regionLable;
    QPushButton *remarkNameButton;
    QPushButton *tagButton;
    QLabel *signatureLabel;
    QLabel *mutualgroupLable;
    QLabel *sourceLabel;

    FriendRequest m_currentFriendRequest;   // 当前显示的新好友申请
    ContactController *m_contactController;
};

#endif // USERINFOWIDGET_H