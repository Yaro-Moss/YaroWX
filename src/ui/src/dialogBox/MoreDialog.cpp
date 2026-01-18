#include "MoreDialog.h"
#include "ui_MoreDialog.h"

MoreDialog::MoreDialog(QWidget *parent)
    : ClickClosePopup(parent)
    , ui(new Ui::MoreDialog)
{
    ui->setupUi(this);
}

MoreDialog::~MoreDialog()
{
    delete ui;
}
