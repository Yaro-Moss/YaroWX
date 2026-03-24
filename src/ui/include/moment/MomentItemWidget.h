#ifndef MOMENTITEMWIDGET_H
#define MOMENTITEMWIDGET_H

#include "CommentInputWidget.h"
#include "Contact.h"
#include "LocalMoment.h"
#include <QWidget>
#include <qpushbutton.h>

namespace Ui {
class MomentItemWidget;
}
class FlowLayout;
class MediaDialog;
class WeChatWidget;
class ContactController;
class LocalMomentController;

class MomentItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MomentItemWidget(QWidget *parent = nullptr);
    ~MomentItemWidget();
    void setLocalMoment(const LocalMoment &localMoment);
    void setWeChatWidget(WeChatWidget* weChatWidget){m_weChatWidget = weChatWidget;}
    void setMediaDialog(MediaDialog* mediaDialog){m_mediaDialog = mediaDialog;}
    void setContactController(ContactController *contactController){m_contactController = contactController;}
    void setLocalMomentController(LocalMomentController *localMomentController){m_localMomentController = localMomentController;}
    void setCurrentUser(const Contact &currentUser);

private slots:
    void on_rightDialogToolButton_clicked();
    // 点赞/评论动作对应的槽函数
    void onLikeActionTriggered();
    void onCommentActionTriggered();
    void onCommentSent(const QString& commentText, const QString &img);

private:
    void createInteractMenu();
    void clearLayout(QLayout *layout);

    Ui::MomentItemWidget *ui;
    Contact m_currentUser;

    LocalMoment m_localMoment;
    FlowLayout *m_mediaFlowLayout;
    FlowLayout *m_likeFlowLayout;

    MediaDialog *m_mediaDialog;
    WeChatWidget *m_weChatWidget;
    ContactController *m_contactController;
    LocalMomentController* m_localMomentController;

    QMenu *m_interactMenu = nullptr;
    QPushButton *likeBtn;
    QPushButton *commentBtn;

    CommentInputWidget *commentInputWidget;
};

#endif // MOMENTITEMWIDGET_H
