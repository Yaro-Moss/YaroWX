#ifndef CUSTOMLISTVIEW_H
#define CUSTOMLISTVIEW_H

#include <QListView>
#include <QTimer>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class CustomListView: public QListView
{
    Q_OBJECT

public:

    explicit CustomListView(QWidget *parent = nullptr);
    ~CustomListView();
    void setMarginRight(int value);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event)override;
    void leaveEvent(QEvent *event)override;
    bool eventFilter(QObject *watched, QEvent *event)override;
    void wheelEvent(QWheelEvent *event)override;
    void resizeEvent(QResizeEvent *event)override;
    void scrollContentsBy(int dx, int dy)override;


private slots:
    void fadeOutOverlay();


private:
    void showOverlayScrollBar();
    void startHideTimer();
    void updateScrollBarPosition();
    void applyScrollBarStyle();
    void onScrollTimerTimeout();

    // 新滚动条显示相关
    QScrollBar *nativeVsb;
    QScrollBar *newVsb;
    QGraphicsOpacityEffect *opacity;
    QPropertyAnimation *fadeAnim;
    QTimer *hideTimer;
    int overlayWidth = 8;
    int marginRight = 2;

    //信号同步锁
    bool isSyncing;

    // 自定义鼠标滚动，精细分帧平滑滚动相关
    int scrollSpeed = 40;         // 滚动一“卡擦”的像素，初始速度
    QTimer *scrollTimer;          // 分帧滚动的定时器
    int remainingScroll;          // 剩余滚动量
    const int scrollStep = 5;     // 每帧滚动的像素
    const int frameInterval = 10; // 每帧间隔（定时器间隔）

};

#endif //CUSTOMLISTVIEW_H
