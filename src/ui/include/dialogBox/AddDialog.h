#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include "AddFriendDialog.h"
#include "ClickClosePopup.h"
#include "ContactController.h"
#include <QPointer>

namespace Ui {
class AddDialog;
}

class AddDialog : public ClickClosePopup
{
    Q_OBJECT

public:
    explicit AddDialog(ContactController * contactController, QWidget *parent = nullptr);
    ~AddDialog();
    void setContactController(ContactController* contactController){m_contactController = contactController;}

private slots:
    void on_addfriendButton_clicked();

private:
    Ui::AddDialog *ui;
    ContactController * m_contactController;
    QPointer<AddFriendDialog> addFriendDialog;
};

#endif // ADDDIALOG_H
