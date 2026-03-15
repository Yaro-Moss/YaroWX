#ifndef CUSTOMTREEVIEW_H
#define CUSTOMTREEVIEW_H

#include <QTreeView>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QTimer>
#include <QGraphicsOpacityEffect>

class CustomTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit CustomTreeView(QWidget *parent = nullptr);
    ~CustomTreeView();

    void applyScrollBarStyle();
    void setMarginRight(int value);

protected:
    void scrollContentsBy(int dx, int dy) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void updateScrollBarPosition();

private slots:
    void fadeOutOverlay();
    void onScrollTimerTimeout();

private:
    void showOverlayScrollBar();
    void startHideTimer();

private:
    // 滚动条相关
    QScrollBar *nativeVsb = nullptr;
    QScrollBar *newVsb = nullptr;
    bool isSyncing = false;
    
    // 淡入淡出效果
    QGraphicsOpacityEffect *opacity = nullptr;
    QPropertyAnimation *fadeAnim = nullptr;
    QTimer *hideTimer = nullptr;
    
    // 平滑滚动
    QTimer *scrollTimer = nullptr;
    int remainingScroll = 0;
    
    // 配置参数
    const int overlayWidth = 8;
    int marginRight = 2;
    const int scrollSpeed = 40;
    const int scrollStep = 4;
    const int frameInterval = 10;
};

#endif // CUSTOMTREEVIEW_H
