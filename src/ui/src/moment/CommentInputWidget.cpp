#include "CommentInputWidget.h"
#include "ui_CommentInputWidget.h"
#include <QDebug>
#include <QFileDialog>
#include <QDebug>
#include <qevent.h>

CommentInputWidget::CommentInputWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CommentInputWidget)
{
    ui->setupUi(this);

    qApp->installEventFilter(this);
}

CommentInputWidget::~CommentInputWidget()
{
    delete ui;
}

void CommentInputWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    QTimer::singleShot(0, this, [=]() {
        ui->inputEdit->setFocus(Qt::ActiveWindowFocusReason);
    });
}

bool CommentInputWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos =mouseEvent->globalPosition().toPoint();        // 鼠标点击的全局坐标
        QPoint localPos = this->mapFromGlobal(globalPos);     // 转换到部件的局部坐标
        QRect widgetRect = this->rect();                      // 部件的局部矩形

        // 如果点击位置不在部件矩形内，则隐藏部件
        if (!widgetRect.contains(localPos)) {
            this->hide();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void CommentInputWidget::on_sendPushButton_clicked()
{
    QString commentText = ui->inputEdit->toPlainText().trimmed();

    if (commentText.isEmpty()&&img.isEmpty())
    {
        qDebug() << "评论为空，不发送";
        return;
    }

    emit commentSent(commentText, img);
    ui->imglabel->setPixmap(QPixmap());
    ui->inputEdit->clear();
    this->hide();
}

void CommentInputWidget::on_emojiButton_clicked()
{
    qDebug() << "点击了表情按钮";
}

void CommentInputWidget::on_imgButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,                          // 父窗口
        tr("选择图片"),                  // 对话框标题
        QDir::homePath(),               // 默认打开路径（例如用户目录）
        tr("Images (*.png *.xpm *.jpg *.jpeg *.bmp *.gif)") // 文件过滤器
        );

    if (!filePath.isEmpty()) {
        img = filePath;  // 保存路径到成员变量
        ui->imglabel->setPixmap(QPixmap(img));
    } else {
        qDebug() << "用户取消了选择";
    }
}
