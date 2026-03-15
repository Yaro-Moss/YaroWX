#include "CustomListView.h"
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPushButton>

CustomListView::CustomListView(QWidget *parent)
    : QListView(parent), isSyncing(false), remainingScroll(0)
{
    // 基本行为
    setSelectionMode(QAbstractItemView::SingleSelection);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);


    // 设置平滑滚动
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // 让视图不占用右侧滚动条布局空间（用新的滚动条）
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 取得native scrollbar(视图实际内部仍使用它进行实际滚动）
    nativeVsb = QListView::verticalScrollBar();
    if (!nativeVsb) nativeVsb = new QScrollBar(Qt::Vertical, this);
    nativeVsb->hide();

    // 创建overly scrollbar （放在viewport上）
    newVsb = new QScrollBar(Qt::Vertical, this->viewport());
    newVsb->setFixedWidth(overlayWidth);
    newVsb->setCursor(Qt::ArrowCursor);

    updateScrollBarPosition();
    applyScrollBarStyle();

    // 设置透明度
    opacity = new QGraphicsOpacityEffect(newVsb);
    opacity->setOpacity(0.0);
    newVsb->setGraphicsEffect(opacity);

    // 设置透明动画，淡进淡出
    fadeAnim = new QPropertyAnimation(opacity, "opacity", this);
    fadeAnim->setDuration(200);
    fadeAnim->setEasingCurve(QEasingCurve::InOutQuad);

    // 隐藏延时
    hideTimer = new QTimer(this);
    hideTimer->setInterval(600);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &CustomListView::fadeOutOverlay);

    // 确保viewport的event/leave/滚轮事件也被捕获
    viewport()->installEventFilter(this);
    installEventFilter(this);
    //当动画结束并且透明的为0时隐藏overlay
    connect(fadeAnim, &QPropertyAnimation::finished, this, [this](){
        if(opacity->opacity()<= 0.001 && newVsb){
            newVsb->hide();
        }
    });

    // native->overlay(视图更新overlay）
    connect(nativeVsb, &QScrollBar::valueChanged, this, [this](int value){
        if(!isSyncing){
            isSyncing = true;
            newVsb->setValue(value);
            isSyncing = false;
        }
    });
    connect(nativeVsb, &QScrollBar::rangeChanged, this, [this](int min, int max){
        newVsb->setRange(min, max);
        newVsb->setPageStep(nativeVsb->pageStep());
        updateScrollBarPosition();
        if(min == max){
            newVsb->hide();
        }else if(newVsb->isHidden()&&(max-min > 0)){
            showOverlayScrollBar();
            startHideTimer();
        }
    });
    connect(nativeVsb, &QScrollBar::sliderMoved, this, [this](int position){
        if(!isSyncing){
            isSyncing = true;
            newVsb->setSliderPosition(position);
            isSyncing = false;
        }
    });

    // voerlay->native(用户拖动overlay时控制真实滚动）
    connect(newVsb, &QScrollBar::valueChanged, this, [this](int value){
        if(!isSyncing){
            isSyncing = true;
            nativeVsb->setValue(value);
            isSyncing = false;
        }
    });
    connect(newVsb, &QScrollBar::sliderMoved, this, [this](int position){
        if(!isSyncing){
            isSyncing = true;
            nativeVsb->setSliderPosition(position);
            isSyncing = false;
        }
    });

    // 初始同步状态
    newVsb->setRange(nativeVsb->minimum(), nativeVsb->maximum());
    newVsb->setPageStep(nativeVsb->pageStep());
    newVsb->setValue(nativeVsb->value());

    // 默认overlay隐藏
    newVsb->hide();

    // 初始化分帧滚动定时器
    scrollTimer = new QTimer(this);
    scrollTimer->setInterval(frameInterval);
    scrollTimer->setSingleShot(false);
    connect(scrollTimer, &QTimer::timeout, this, &CustomListView::onScrollTimerTimeout);
}

CustomListView::~CustomListView()
{
    hideTimer->stop();
    scrollTimer->stop();
}

// 更新样式
void CustomListView::applyScrollBarStyle()
{
    if(!newVsb) return;
    newVsb->setStyleSheet(QString(
        "QScrollBar:vertical{"
        "background: transparent;"
        "margin: 0px;"
        "border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical{"
        "background: rgba(150,150,150,220);"
        "border-radius: 4px;"
        "min-height: 60px;"
        "}"
        "QScrollBar::handle:vertical:hover{"
        "background: rgba(140,140,140,220);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "subcontrol-origin: margin;"
        "background: transparent;"
        "height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "background: transparent;"
        "}"
        ));
}

void CustomListView::scrollContentsBy(int dx, int dy)
{
    QListView::scrollContentsBy(dx, dy);
    updateScrollBarPosition();
}

void CustomListView::updateScrollBarPosition()
{
    if(!newVsb || !viewport()) return;
    QRect vpRect = viewport()->rect();
    int x = vpRect.right() - overlayWidth - marginRight;
    int y = vpRect.top();
    int w = overlayWidth;
    int h = vpRect.height();

    newVsb->setGeometry(x,y,w,h);
    newVsb->raise();
}

void CustomListView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QModelIndex index = indexAt(event->pos());
        if(index.isValid()){
            if(selectionModel()->isSelected(index)){
                selectionModel()->select(index, QItemSelectionModel::Deselect);
                emit clicked(index);
                return;
            }
        }
    }
    // 点击时显示滚动条
    showOverlayScrollBar();
    QListView::mousePressEvent(event);
}

void CustomListView::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    showOverlayScrollBar();
    hideTimer->stop();
    QListView::enterEvent(event);
}

void CustomListView::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    startHideTimer();
    QListView::leaveEvent(event);
}

bool CustomListView::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == viewport()){
        if(event->type() == QEvent::Enter){
            showOverlayScrollBar();
            hideTimer->stop();
        }else if(event->type() == QEvent::Leave){
            startHideTimer();
        }else if(event->type() == QEvent::Wheel){
            showOverlayScrollBar();
        }
    }
    return QListView::eventFilter(watched, event);
}

void CustomListView::wheelEvent(QWheelEvent *event)
{
    showOverlayScrollBar();
    int totalScrollPixel = 0;
    const qreal ANGLE_TO_PIXEL = 0.17;

    if(event->pixelDelta().y() != 0){
        //触控板：直接使用像素增量
        totalScrollPixel = event->pixelDelta().y();
    }
    else{
        // 鼠标滚轮：角度转像素
        const qreal angle = event->angleDelta().y();
        const qreal scrollPixel = angle * ANGLE_TO_PIXEL;

        // 滚动越多速度越快
        qreal mul = qMin(qAbs(remainingScroll)/scrollSpeed, 2);
        qreal scrollSpeed_1 = scrollSpeed + mul*scrollSpeed;

        const qreal finalScrollPixel = scrollPixel * (scrollSpeed_1/20.0);
        totalScrollPixel = qRound(finalScrollPixel);
    }
    // 添加到滚动量
    remainingScroll += -totalScrollPixel;
    if(!scrollTimer->isActive()) scrollTimer->start();

    event->accept();
}

void CustomListView::resizeEvent(QResizeEvent *event)
{
    QListView::resizeEvent(event);
    updateScrollBarPosition();

    updateGeometries();

    // 如果有模型，触发数据改变信号来刷新所有可见项
    if (model()) {
        // 获取可见区域的首尾索引
        QModelIndex firstVisible = indexAt(viewport()->rect().topLeft());
        QModelIndex lastVisible = indexAt(viewport()->rect().bottomLeft());

        if (firstVisible.isValid() && lastVisible.isValid()) {
            // 触发可见区域的数据改变信号
            emit model()->dataChanged(firstVisible, lastVisible);
        }
    }
}

void CustomListView::showOverlayScrollBar()
{
    if(!newVsb) return;
    hideTimer->stop();
    if(!newVsb->isVisible()) newVsb->show();

    // 如果已经完全不透明，不需要启动动画
    if(qFuzzyCompare(opacity->opacity(), 1.0)) return;

    fadeAnim->stop();
    fadeAnim->setStartValue(opacity->opacity());
    fadeAnim->setEndValue(1.0);
    fadeAnim->start();
}

void CustomListView::startHideTimer()
{
    if(!newVsb) return;
    hideTimer->start();
}

void CustomListView::fadeOutOverlay()
{
    if(!newVsb) return;

    fadeAnim->stop();
    fadeAnim->setStartValue(opacity->opacity());
    fadeAnim->setEndValue(0.0);
    fadeAnim->start();
}

// 定时器触发的小幅度滚动函数
void CustomListView::onScrollTimerTimeout()
{
    //剩余滚动量为0，停止定时器
    if(remainingScroll == 0){
        scrollTimer->stop();
        return;
    }
    // 滚动量存的越多速度越快
    int mul = qAbs(remainingScroll)/scrollSpeed;
    int step = scrollStep + mul*scrollStep;

    // 保持滚动方向（剩余量为正--下滚， 负--上滚）
    if(remainingScroll<0) step = -step;

    // 更新原生滚动条的vlue（驱动视图滚动）
    if(nativeVsb){
        int newVlue = nativeVsb->value() + step;
        newVlue = qMax(nativeVsb->minimum(), qMin(nativeVsb->maximum(),newVlue));
        nativeVsb->setValue(newVlue);
    }

    // 减少剩余滚动量
    remainingScroll -= step;
    if(qAbs(remainingScroll) < scrollStep) remainingScroll = 0;
}

void CustomListView::setMarginRight(int value)
{
    marginRight = value;
}



