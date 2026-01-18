#ifndef USERINFOWIDGET_H
#define USERINFOWIDGET_H

#include "Contact.h"
#include "ImgLabel.h"
#include <QWidget>
#include <QPushButton>

namespace Ui {
class UserInfoWidget;
}

class UserInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UserInfoWidget(QWidget *parent = nullptr);
    ~UserInfoWidget();

    void setSelectedContact(const Contact &contact);

    ImgLabel *avatarLabel;

signals:
    void switchtoMessageInterface(const Contact &contact);



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

};

#endif // USERINFOWIDGET_H
