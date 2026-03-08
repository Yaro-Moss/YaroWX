#include "FloatingDialog.h"
#include "ui_FloatingDialog.h"

FloatingDialog::FloatingDialog(QWidget *parent)
    : ClickClosePopup(parent)
    , ui(new Ui::FloatingDialog)
{
    ui->setupUi(this);
    enableClickCloseFeature();

}

FloatingDialog::~FloatingDialog()
{
    delete ui;
}
