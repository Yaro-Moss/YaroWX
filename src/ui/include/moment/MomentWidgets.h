#ifndef MOMENTWIDGETS_H
#define MOMENTWIDGETS_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QList>
#include <QDateTime>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QPixmap>
#include <QFrame>
#include <QFileDialog>
#include <QHash>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QStandardPaths>
#include <QFile>
#include <QRandomGenerator>
#include "LocalMoment.h"
#include "LocalMomentController.h"
#include "MomentEditWidget.h"
#include "MomentItemWidget.h"
#include<QPointer>
#include <QMenu>
#include <QEvent>


namespace Ui {
class MomentMainWidget;
}
class MediaDialog;
class WeChatWidget;
class ContactController;

class MomentMainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MomentMainWidget(LocalMomentController* controller, QWidget *parent = nullptr);
    ~MomentMainWidget();
    void setWeChatWidget(WeChatWidget* weChatWidget);
    void setMediaDialog(MediaDialog* mediaDialog);
    void setContactController(ContactController *contactController){m_contactController = contactController;}
    void setCurrentUser(const Contact &currentUser);
    void setLocalMomentController(LocalMomentController *localMomentController){m_localMomentController = localMomentController;}

protected:
    // 事件过滤器重写
    bool eventFilter(QObject *watched, QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onScrollBarValueChanged(int value);
    void on_minWinButton_clicked();
    void on_closeButton_clicked();
    void on_addMomentButton_clicked();

    // 菜单项对应的槽函数
    void onSelectPhotoVideoTriggered();  // 选择照片/视频
    void onPublishTextTriggered();      // 发表文字
    void on_refreshButton_clicked();

private:
    void addMoment(const QString &text, const QStringList &selectedFilePaths, FileType fileType);
    void initializeMenu();

    // 工具函数
    MomentItemWidget* createMomentItemWidget(const LocalMoment &localMoment);
    void showMoments();

    Ui::MomentMainWidget *ui;
    QScrollArea* m_scrollArea;
    LocalMomentController* m_localMomentController;
    ContactController *m_contactController;
    QPointer<MomentEditWidget> m_momentEditWidget;
    QVBoxLayout* m_momentListLayout;

    // 拖动/拉伸核心变量
    QPoint m_dragStartPos;
    bool m_isDragging = false;
    bool m_isResizingTop = false;
    bool m_isResizingBottom = false;
    const int m_resizeMargin = 3;
    const int m_topBarHeight = 35;
    const int m_minWindowHeight = 300;

    // 右键菜单成员变量
    QMenu* m_rightClickMenu;

    // 朋友圈动态相关数据
    QVector<LocalMoment> m_moments;        // 当前加载的朋友圈列表
    QVector<MomentItemWidget*> momentItemWidgets;
    QHash<qint64,int> m_momentsIndexs;      // 朋友圈缓存的索引用于快速拿取某个朋友圈
    bool m_hasMore;

    MediaDialog *m_mediaDialog;
    WeChatWidget *m_weChatWidget;

    Contact m_currentUser;

};


#endif // MOMENTWIDGETS_H
