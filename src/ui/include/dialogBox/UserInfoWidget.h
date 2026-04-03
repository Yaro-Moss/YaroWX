#ifndef USERINFOWIDGET_H
#define USERINFOWIDGET_H

#include "Contact.h"
#include "ImgLabel.h"
#include <QWidget>
#include <QPushButton>

namespace Ui {
class UserInfoWidget;
}

class MediaDialog;
class WeChatWidget;

class UserInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UserInfoWidget(QWidget *parent = nullptr);
    ~UserInfoWidget();

    void setSelectedContact(const Contact &contact);
    void setWeChatWidget(WeChatWidget* weChatWidget){m_weChatWidget = weChatWidget;}
    void setMediaDialog(MediaDialog* mediaDialog);

private slots:
    void on_switchMessageInterfaceToolButton_clicked();

private:
    Ui::UserInfoWidget *ui;
    Contact m_contact;

    QLabel *remarkNameLable;
    QLabel *nicknameLable;
    QLabel *accountLable;
    QLabel *regionLable;
    QPushButton *remarkNameButton;
    QPushButton *tagButton;
    QLabel *signatureLabel;
    QLabel *mutualgroupLable;
    QLabel *sourceLabel;
    ImgLabel *avatarLabel;

    MediaDialog *m_mediaDialog;
    WeChatWidget *m_weChatWidget;
};

#endif // USERINFOWIDGET_H
