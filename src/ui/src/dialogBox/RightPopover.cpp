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

void RightPopover::setWeChatWidget(WeChatWidget* weChatWidget)
{
    m_weChatWidget = weChatWidget;
    ui->rightAvatarButton->setWeChatWidget(m_weChatWidget);
}

void RightPopover::setMediaDialog(MediaDialog* mediaDialog)
{
    m_mediaDialog = mediaDialog;
    ui->rightAvatarButton->setMediaDialog(m_mediaDialog);
}

void RightPopover::setContact(const Contact &contact)
{
    Contact m_contact = contact;
    ui->rightAvatarButton->setContact(contact);
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

