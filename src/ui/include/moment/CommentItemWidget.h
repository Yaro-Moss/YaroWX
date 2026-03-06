#ifndef COMMENTITEMWIDGET_H
#define COMMENTITEMWIDGET_H

#include "Contact.h"
#include "LocalMoment.h"
#include "MediaDialog.h"
#include "WeChatWidget.h"
#include <QWidget>

namespace Ui {
class CommentItemWidget;
}

class CommentItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommentItemWidget(QWidget *parent = nullptr);
    ~CommentItemWidget();
    void setContact(const Contact& contact){m_ontact = contact;}
    void setMomentCommentInfo(const MomentCommentInfo& momentCommentInfo);
    void setIcon();
    void setWeChatWidget(WeChatWidget* weChatWidget){m_weChatWidget = weChatWidget;}
    void setMediaDialog(MediaDialog* mediaDialog){m_mediaDialog = mediaDialog;}
    void setContactController(ContactController *contactController){m_contactController = contactController;}


private slots:
    void on_commentAvatarButton_clicked();

    void on_userNameButton_clicked();

private:
    Ui::CommentItemWidget *ui;
    MediaDialog *m_mediaDialog;
    WeChatWidget *m_weChatWidget;
    ContactController *m_contactController;
    Contact m_ontact;
    MomentCommentInfo m_momentCommentInfo;
};

#endif // COMMENTITEMWIDGET_H
