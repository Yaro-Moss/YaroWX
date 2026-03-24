#include "AddFriendDialog.h"
#include "StrangerWidget.h"
#include "ui_AddFriendDialog.h"
#include"GenerationWorker.h"
#include <qpainter.h>
#include <QDebug>  // 可选，用于调试

AddFriendDialog::AddFriendDialog(ContactController *contactController, QWidget *parent)
    : QDialog(parent), m_contactController(contactController)
    , ui(new Ui::AddFriendDialog)
    // 初始化拖动变量
    , m_isDragging(false)
    , m_titleBarHeight(20)
{
    ui->setupUi(this);
    strangerWidget = ui->strangerWidget;
    strangerWidget->hide();

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);  // 开启鼠标追踪

    setAttribute(Qt::WA_DeleteOnClose);
    connect(strangerWidget, &StrangerWidget::addFriend, this, &AddFriendDialog::addFriend);

    //-------暂时测试------------
    generationWorker = new GenerationWorker(this);
}

AddFriendDialog::~AddFriendDialog()
{
    delete ui;
}

void AddFriendDialog::on_closeButton_clicked()
{
    close();
}

void AddFriendDialog::on_searchButton_clicked()
{
    strangerWidget->hide();

    User user = generationWorker->generateSingleUser();
    if(!user.isValid())qDebug()<<"生成用户失败.";

    QString num = ui->searchLineEdit->text();
    ui->searchLineEdit->clear();
    if(!num.isEmpty()){
        user.setaccount(num);
    }

    strangerWidget->updateUser(user);
    strangerWidget->show();
}

void AddFriendDialog::addFriend(){
    addFriendRequestDialog = new AddFriendRequestDialog(m_contactController);
    addFriendRequestDialog->show();
}

void AddFriendDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(160,160,160),1);
    painter.setPen(pen);
    painter.setBrush(QColor(248,248,248));

    QRect rect = QRect(1,1,width()-2,height()-2);
    painter.drawRoundedRect(rect, 10, 10);
}

void AddFriendDialog::mousePressEvent(QMouseEvent *event)
{
    // 仅处理左键，且点击位置在顶部可拖动区域内（y坐标 < 顶部高度）
    if (event->button() == Qt::LeftButton && event->pos().y() < m_titleBarHeight) {
        m_isDragging = true;
        // 记录鼠标按下时相对于窗口的坐标（而非屏幕坐标）
        m_dragStartPos = event->pos();
    }
    QDialog::mousePressEvent(event);
}

void AddFriendDialog::mouseMoveEvent(QMouseEvent *event)
{
    // 仅当拖动标记为true，且左键按住时执行拖动
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint newWindowPos = event->globalPosition().toPoint() - m_dragStartPos;
        this->move(newWindowPos);
    }
    QDialog::mouseMoveEvent(event);
}

void AddFriendDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}
