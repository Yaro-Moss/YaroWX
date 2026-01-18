#include "AddDialog.h"
#include "ui_AddDialog.h"

AddDialog::AddDialog(QWidget *parent)
    : ClickClosePopup(parent)
    , ui(new Ui::AddDialog)
{
    ui->setupUi(this);
}

AddDialog::~AddDialog()
{
    delete ui;
}
