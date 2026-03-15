#include "CustomTreeView.h"
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QEnterEvent>

CustomTreeView::CustomTreeView(QWidget *parent)
    : QTreeView(parent)
{
    // 基本行为
    setSelectionMode(QAbstractItemView::SingleSelection);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // 设置平滑滚动
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    
    // 隐藏原生滚动条，使用自定义的
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 获取原生垂直滚动条
    nativeVsb = QTreeView::verticalScrollBar();
    if (!nativeVsb) nativeVsb = new QScrollBar(Qt::Vertical, this);
    nativeVsb->hide();
    
    // 创建自定义覆盖滚动条
    newVsb = new QScrollBar(Qt::Vertical, this->viewport());
    newVsb->setFixedWidth(overlayWidth);
    newVsb->setCursor(Qt::ArrowCursor);
    
    updateScrollBarPosition();
    applyScrollBarStyle();
    
    // 透明度效果
    opacity = new QGraphicsOpacityEffect(newVsb);
    opacity->setOpacity(0.0);
    newVsb->setGraphicsEffect(opacity);
    
    // 淡入淡出动画
    fadeAnim = new QPropertyAnimation(opacity, "opacity", this);
    fadeAnim->setDuration(200);
    fadeAnim->setEasingCurve(QEasingCurve::InOutQuad);
    
    // 隐藏定时器
    hideTimer = new QTimer(this);
    hideTimer->setInterval(600);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &CustomTreeView::fadeOutOverlay);
    
    // 事件过滤器
    viewport()->installEventFilter(this);
    installEventFilter(this);
    
    // 动画结束时隐藏滚动条
    connect(fadeAnim, &QPropertyAnimation::finished, this, [this]() {
        if (opacity->opacity() <= 0.001 && newVsb) {
            newVsb->hide();
        }
    });
    
    // 原生滚动条 -> 自定义滚动条同步
    connect(nativeVsb, &QScrollBar::valueChanged, this, [this](int value) {
        if (!isSyncing) {
            isSyncing = true;
            newVsb->setValue(value);
            isSyncing = false;
        }
    });
    
    connect(nativeVsb, &QScrollBar::rangeChanged, this, [this](int min, int max) {
        newVsb->setRange(min, max);
        newVsb->setPageStep(nativeVsb->pageStep());
        updateScrollBarPosition();
        if (min == max) {
            newVsb->hide();
        } else if (newVsb->isHidden() && (max - min > 0)) {
            showOverlayScrollBar();
            startHideTimer();
        }
    });
    
    connect(nativeVsb, &QScrollBar::sliderMoved, this, [this](int position) {
        if (!isSyncing) {
            isSyncing = true;
            newVsb->setSliderPosition(position);
            isSyncing = false;
        }
    });
    
    // 自定义滚动条 -> 原生滚动条同步
    connect(newVsb, &QScrollBar::valueChanged, this, [this](int value) {
        if (!isSyncing) {
            isSyncing = true;
            nativeVsb->setValue(value);
            isSyncing = false;
        }
    });
    
    connect(newVsb, &QScrollBar::sliderMoved, this, [this](int position) {
        if (!isSyncing) {
            isSyncing = true;
            nativeVsb->setSliderPosition(position);
            isSyncing = false;
        }
    });
    
    // 初始同步
    newVsb->setRange(nativeVsb->minimum(), nativeVsb->maximum());
    newVsb->setPageStep(nativeVsb->pageStep());
    newVsb->setValue(nativeVsb->value());
    
    // 默认隐藏
    newVsb->hide();
    
    // 平滑滚动定时器
    scrollTimer = new QTimer(this);
    scrollTimer->setInterval(frameInterval);
    scrollTimer->setSingleShot(false);
    connect(scrollTimer, &QTimer::timeout, this, &CustomTreeView::onScrollTimerTimeout);
}

CustomTreeView::~CustomTreeView()
{
    hideTimer->stop();
    scrollTimer->stop();
}

void CustomTreeView::applyScrollBarStyle()
{
    if (!newVsb) return;

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
void CustomTreeView::scrollContentsBy(int dx, int dy)
{
    QTreeView::scrollContentsBy(dx, dy);
    updateScrollBarPosition();
}

void CustomTreeView::updateScrollBarPosition()
{
    if (!newVsb || !viewport()) return;
    
    QRect vpRect = viewport()->rect();
    int x = vpRect.right() - overlayWidth - marginRight;
    int y = vpRect.top();
    int w = overlayWidth;
    int h = vpRect.height();
    
    newVsb->setGeometry(x, y, w, h);
    newVsb->raise();
}

void CustomTreeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            if (selectionModel()->isSelected(index)) {
                selectionModel()->select(index, QItemSelectionModel::Deselect);
                emit clicked(index);
                return;
            }
        }
    }
    
    showOverlayScrollBar();
    QTreeView::mousePressEvent(event);
}

void CustomTreeView::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    showOverlayScrollBar();
    hideTimer->stop();
    QTreeView::enterEvent(event);
}

void CustomTreeView::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    startHideTimer();
    QTreeView::leaveEvent(event);
}

bool CustomTreeView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == viewport()) {
        if (event->type() == QEvent::Enter) {
            showOverlayScrollBar();
            hideTimer->stop();
        } else if (event->type() == QEvent::Leave) {
            startHideTimer();
        } else if (event->type() == QEvent::Wheel) {
            showOverlayScrollBar();
        }
    }
    return QTreeView::eventFilter(watched, event);
}

void CustomTreeView::wheelEvent(QWheelEvent *event)
{
    showOverlayScrollBar();
    
    int totalScrollPixel = 0;
    const qreal ANGLE_TO_PIXEL = 0.17;
    
    if (event->pixelDelta().y() != 0) {
        // 触控板
        totalScrollPixel = event->pixelDelta().y();
    } else {
        // 鼠标滚轮
        const qreal angle = event->angleDelta().y();
        const qreal scrollPixel = angle * ANGLE_TO_PIXEL;
        
        // 滚动速度计算
        qreal mul = qMin(qreal(qAbs(remainingScroll) / scrollSpeed), qreal(2.0));
        qreal scrollSpeed_1 = scrollSpeed + mul * scrollSpeed;
        
        const qreal finalScrollPixel = scrollPixel * (scrollSpeed_1 / 20.0);
        totalScrollPixel = qRound(finalScrollPixel);
    }
    
    remainingScroll += -totalScrollPixel;
    if (!scrollTimer->isActive()) scrollTimer->start();
    
    event->accept();
}

void CustomTreeView::resizeEvent(QResizeEvent *event)
{
    QTreeView::resizeEvent(event);
    updateScrollBarPosition();
    
    // 触发几何更新
    updateGeometries();
    
    // 刷新可见项
    if (model()) {
        QModelIndex firstVisible = indexAt(viewport()->rect().topLeft());
        QModelIndex lastVisible = indexAt(viewport()->rect().bottomLeft());
        
        if (firstVisible.isValid() && lastVisible.isValid()) {
            emit model()->dataChanged(firstVisible, lastVisible);
        }
    }
}

void CustomTreeView::showOverlayScrollBar()
{
    if (!newVsb) return;
    
    hideTimer->stop();
    if (!newVsb->isVisible()) newVsb->show();
    
    if (qFuzzyCompare(opacity->opacity(), 1.0)) return;
    
    fadeAnim->stop();
    fadeAnim->setStartValue(opacity->opacity());
    fadeAnim->setEndValue(1.0);
    fadeAnim->start();
}

void CustomTreeView::startHideTimer()
{
    if (!newVsb) return;
    hideTimer->start();
}

void CustomTreeView::fadeOutOverlay()
{
    if (!newVsb) return;
    
    fadeAnim->stop();
    fadeAnim->setStartValue(opacity->opacity());
    fadeAnim->setEndValue(0.0);
    fadeAnim->start();
}

void CustomTreeView::onScrollTimerTimeout()
{
    if (remainingScroll == 0) {
        scrollTimer->stop();
        return;
    }
    
    int mul = qAbs(remainingScroll) / scrollSpeed;
    int step = scrollStep + mul * scrollStep;
    
    if (remainingScroll < 0) step = -step;
    
    if (nativeVsb) {
        int newValue = nativeVsb->value() + step;
        newValue = qMax(nativeVsb->minimum(), qMin(nativeVsb->maximum(), newValue));
        nativeVsb->setValue(newValue);
    }
    
    remainingScroll -= step;
    if (qAbs(remainingScroll) < scrollStep) remainingScroll = 0;
}

void CustomTreeView::setMarginRight(int value)
{
    marginRight = value;
    updateScrollBarPosition();
}
