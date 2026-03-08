#ifndef CURRENTUSERINFODIALOG_H
#define CURRENTUSERINFODIALOG_H

#include "ClickClosePopup.h"
#include "Contact.h"
#include <QLabel>

namespace Ui {
class CurrentUserInfoDialog;
}
class ImgLabel;
class WeChatWidget;
class MediaDialog;

class CurrentUserInfoDialog : public ClickClosePopup
{
    Q_OBJECT

public:
    explicit CurrentUserInfoDialog(QWidget *parent = nullptr);
    ~CurrentUserInfoDialog();
    void setCurrentUser(const Contact &user);
    void setWeChatWidget(WeChatWidget* weChatWidget){m_weChatWidget = weChatWidget;}
    void setMediaDialog(MediaDialog* mediaDialog);
    void setPixmap(const QPixmap &pixmap);

protected:


private slots:
    void on_switchMessageInterfaceToolButton_clicked();

private:
    Ui::CurrentUserInfoDialog *ui;

    ImgLabel *avatarLabel;
    QLabel *account;
    QLabel *region;
    QLabel *nickname;

    Contact currentUser;
    MediaDialog *m_mediaDialog;
    WeChatWidget *m_weChatWidget;

};

#endif // CURRENTUSERINFODIALOG_H
