#include "MoreDialog.h"
#include "ui_MoreDialog.h"

MoreDialog::MoreDialog(QWidget *parent)
    : ClickClosePopup(parent)
    , ui(new Ui::MoreDialog)
{
    ui->setupUi(this);
    enableClickCloseFeature();

}

MoreDialog::~MoreDialog()
{
    delete ui;
}
