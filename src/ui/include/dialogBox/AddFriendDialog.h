#ifndef ADDFRIENDDIALOG_H
#define ADDFRIENDDIALOG_H

#include "AddFriendRequestDialog.h"
#include "ContactController.h"
#include "GenerationWorker.h"
#include "StrangerWidget.h"
#include <QDialog>
#include <QPointer>
#include <QMouseEvent>

namespace Ui {
class AddFriendDialog;
}

class AddFriendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFriendDialog(ContactController* contactController, QWidget *parent = nullptr);
    ~AddFriendDialog();

private slots:
    void on_closeButton_clicked();
    void on_searchButton_clicked();
    void addFriend();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::AddFriendDialog *ui;
    ContactController * m_contactController;
    QPointer<AddFriendRequestDialog> addFriendRequestDialog;
    StrangerWidget * strangerWidget;
    GenerationWorker *generationWorker;

    bool m_isDragging;          // 标记是否正在拖动窗口
    QPoint m_dragStartPos;      // 鼠标按下时相对于窗口的位置
    int m_titleBarHeight;       // 顶部可拖动区域的高度
};

#endif // ADDFRIENDDIALOG_H
