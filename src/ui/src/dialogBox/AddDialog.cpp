#include "AddDialog.h"
#include "ui_AddDialog.h"

AddDialog::AddDialog(ContactController * contactController, QWidget *parent)
    : ClickClosePopup(parent), m_contactController(contactController)
    , ui(new Ui::AddDialog)
{
    ui->setupUi(this);
}

AddDialog::~AddDialog()
{
    delete ui;
}

void AddDialog::on_addfriendButton_clicked()
{
    if(!addFriendDialog) addFriendDialog = new AddFriendDialog(m_contactController);

    addFriendDialog->show();

    this->deleteLater();
}

