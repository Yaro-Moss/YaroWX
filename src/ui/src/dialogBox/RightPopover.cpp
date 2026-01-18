#include "RightPopover.h"
#include "ui_RightPopover.h"
#include<QDebug>
#include <QMouseEvent>

RightPopover::RightPopover(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RightPopover)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);
}

RightPopover::~RightPopover()
{
    delete ui;
    qApp->removeEventFilter(this);

}

bool RightPopover::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (isVisible()) {
            QPoint localPos = mapFromGlobal(mouseEvent->globalPosition().toPoint());
            if (!rect().contains(localPos)) {
                emit cloesDialog();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}


void RightPopover::on_pushButton_4_clicked()
{
    qDebug()<<"点击按钮";
}

