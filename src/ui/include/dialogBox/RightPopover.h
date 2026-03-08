#ifndef RIGHTPOPOVER_H
#define RIGHTPOPOVER_H

#include "Contact.h"
#include <QWidget>

namespace Ui {
class RightPopover;
}
class WeChatWidget;
class MediaDialog;

class RightPopover : public QWidget
{
    Q_OBJECT

public:
    explicit RightPopover(QWidget *parent = nullptr);
    ~RightPopover();
    void setWeChatWidget(WeChatWidget* weChatWidget);
    void setMediaDialog(MediaDialog* mediaDialog);
    void setContact(const Contact &contact);

signals:
    void cloesDialog();


protected:
    bool eventFilter(QObject *obj, QEvent *event) override;



private slots:

    void on_pushButton_4_clicked();



private:
    Ui::RightPopover *ui;
    WeChatWidget *m_weChatWidget;
    MediaDialog *m_mediaDialog;
    Contact m_contact;

};

#endif // RIGHTPOPOVER_H
