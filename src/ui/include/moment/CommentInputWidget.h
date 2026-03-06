#ifndef COMMENTINPUTWIDGET_H
#define COMMENTINPUTWIDGET_H

#include "Contact.h"
#include <QWidget>
#include <QEvent>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class CommentInputWidget; }
QT_END_NAMESPACE

class CommentInputWidget : public QWidget
{
    Q_OBJECT

signals:
    void commentSent(const QString& commentText, const QString &img);

public:
    CommentInputWidget(QWidget *parent = nullptr);
    ~CommentInputWidget();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;


private slots:
    void on_sendPushButton_clicked();
    void on_emojiButton_clicked();
    void on_imgButton_clicked();

private:
    Ui::CommentInputWidget *ui;
    QString img;
};

#endif // COMMENTINPUTWIDGET_H
