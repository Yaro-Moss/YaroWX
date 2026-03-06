#ifndef STRANGERWIDGET_H
#define STRANGERWIDGET_H

#include "User.h"
#include <QWidget>

namespace Ui {
class StrangerWidget;
}

class StrangerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StrangerWidget(QWidget *parent = nullptr);
    ~StrangerWidget();

    void updateUser(const User& user);

signals:
    void addFriend(const User& user);

private slots:
    void on_addButton_clicked();

private:
    Ui::StrangerWidget *ui;
    User m_user;
};

#endif // STRANGERWIDGET_H
