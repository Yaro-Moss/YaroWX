#include "AddFriendRequestDialog.h"
#include "ui_AddFriendRequestDialog.h"
#include <qpainter.h>
#include <QDebug>
#include <QMessageBox>

AddFriendRequestDialog::AddFriendRequestDialog(ContactController *contactController, QWidget *parent)
    : QDialog(parent), m_contactController(contactController)
    , ui(new Ui::AddFriendRequestDialog)
    // 初始化拖动变量
    , m_isDragging(false)
    , m_titleBarHeight(20)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

AddFriendRequestDialog::~AddFriendRequestDialog()
{
    delete ui;
}

void AddFriendRequestDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(160,160,160),1);
    painter.setPen(pen);
    painter.setBrush(QColor(238,238,238));

    QRect rect = QRect(1,1,width()-2,height()-2);
    painter.drawRoundedRect(rect, 8, 8);
}

void AddFriendRequestDialog::mousePressEvent(QMouseEvent *event)
{
    // 仅处理左键，且点击位置在顶部可拖动区域内（y坐标 < 顶部高度）
    if (event->button() == Qt::LeftButton && event->pos().y() < m_titleBarHeight) {
        m_isDragging = true;
        // 记录鼠标按下时相对于窗口的坐标（而非屏幕坐标）
        m_dragStartPos = event->pos();
    }
    QDialog::mousePressEvent(event);
}

void AddFriendRequestDialog::mouseMoveEvent(QMouseEvent *event)
{
    // 仅当拖动标记为true，且左键按住时执行拖动
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // 计算窗口新位置：屏幕鼠标位置 - 按下时相对窗口的位置
        QPoint newWindowPos = event->globalPosition().toPoint() - m_dragStartPos;
        this->move(newWindowPos);
    }
    QDialog::mouseMoveEvent(event);
}

void AddFriendRequestDialog::mouseReleaseEvent(QMouseEvent *event)
{
    // 释放左键时停止拖动
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void AddFriendRequestDialog::on_closeButton_clicked()
{
    close();
}




void AddFriendRequestDialog::on_cancelButton_clicked()
{
    close();
}



void AddFriendRequestDialog::on_confirmButton_clicked()
{
    // 1. 获取输入内容，trimmed() 去除首尾空格
    QString inputText = ui->greetingTextEdit->toPlainText().trimmed();

    // 2. 判空校验：
    if (inputText.isEmpty()) {
        QMessageBox::warning(
            this,
            tr("警告"),
            tr("打招呼内容不能为空，请输入后再提交！")
            );
        return;
    }

    const int maxLength = 100;
    if (inputText.length() > maxLength) {
        QMessageBox::warning(
            this,
            tr("警告"),
            tr("打招呼内容不能超过 %1 字，请精简后再提交！").arg(maxLength)
            );
        return;
    }

    qDebug() << "好友申请招呼内容：" << inputText;
    this->accept();
}
