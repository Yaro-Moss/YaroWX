#ifndef ADDFRIENDREQUESTDIALOG_H
#define ADDFRIENDREQUESTDIALOG_H

#include "ContactController.h"
#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class AddFriendRequestDialog;
}

class AddFriendRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFriendRequestDialog(ContactController * contactController, QWidget *parent = nullptr);
    ~AddFriendRequestDialog();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_closeButton_clicked();


    void on_cancelButton_clicked();

    void on_confirmButton_clicked();

private:
    Ui::AddFriendRequestDialog *ui;
    ContactController * m_contactController;

    bool m_isDragging;          // 标记是否正在拖动窗口
    QPoint m_dragStartPos;      // 鼠标按下时相对于窗口的位置
    int m_titleBarHeight;       // 顶部可拖动区域的高度
};

#endif // ADDFRIENDREQUESTDIALOG_H
