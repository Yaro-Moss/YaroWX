#include "CustomGraphicsView.h"
#include <QMouseEvent>
#include <QGraphicsPixmapItem>
#include <QScrollBar>

CustomGraphicsView::CustomGraphicsView(QWidget *parent)
    :QGraphicsView(parent),
    m_scene(new QGraphicsScene(this)),
    m_pixmapItem(nullptr),
    m_scaleFactor(1.0),
    m_wasFitToWindow(false)

{
    m_scene->setBackgroundBrush(QBrush(QColor(252, 252, 252)));
    setScene(m_scene);
    //设置视图属性
    setDragMode(NoDrag);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorUnderMouse);

}


//统一清理函数
void CustomGraphicsView::clearCurrentImage()
{
    if(m_pixmapItem){
        m_scene->removeItem(m_pixmapItem);
        delete m_pixmapItem;
        m_pixmapItem = nullptr;
    }
}


void CustomGraphicsView::loadImage(const QString &imagePath)
{
    clearCurrentImage();//清理原图
    //加载新图片
    m_originalPixmap.load(imagePath);
    if(!m_originalPixmap.isNull()){
        m_pixmapItem = m_scene->addPixmap(m_originalPixmap);
        m_scene->setSceneRect(m_originalPixmap.rect());
        resetTransform();
        m_scaleFactor = 1.0;
        fitImageToWindow();
        m_wasFitToWindow = true;
    }
}


void CustomGraphicsView::loadImage(const QPixmap &pixmap)
{
    clearCurrentImage();//清理原图
    if(!pixmap.isNull()){
        m_originalPixmap = pixmap;//保存像素图
        m_pixmapItem = m_scene->addPixmap(m_originalPixmap);
        m_scene->setSceneRect(m_originalPixmap.rect());
        resetTransform();
        m_scaleFactor = 1.0;
        fitImageToWindow();
        m_wasFitToWindow = true;
    }
}


void CustomGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && !m_originalPixmap.isNull())
    {
        setCursor(Qt::ClosedHandCursor);
        setDragMode(ScrollHandDrag);
    }
    QGraphicsView::mousePressEvent(event);
}


void CustomGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        setCursor(Qt::ArrowCursor);
        setDragMode(NoDrag);
    }
    QGraphicsView::mouseReleaseEvent(event);
}


void CustomGraphicsView::wheelEvent(QWheelEvent *event)
{
    if(!m_originalPixmap.isNull()){
        if(event->angleDelta().y()>0){
            zoomTo(m_scaleFactor*1.10, event->position().toPoint());
        }else{
            zoomTo(m_scaleFactor*0.90, event->position().toPoint());
        }
        event->accept();
        m_wasFitToWindow = false;
    }
    else
    {
        QGraphicsView::wheelEvent(event);
    }
}


void CustomGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if(m_wasFitToWindow){
        fitImageToWindow();
    }
}

// 放缩函数
void CustomGraphicsView::zoomTo(qreal scale, const QPointF &center)
{
    // 限制缩放范围
    const qreal minScale = 0.03;
    const qreal maxScale = 200.0;
    scale = qBound(minScale, scale, maxScale);
    //保存储当前中心点
    QPointF oldPos = mapToScene(center.x(), center.y());
    //应用放缩
    m_scaleFactor = scale;
    setTransform(QTransform::fromScale(scale,scale));

    // 计算新的中心点在场景中的位置
    QPointF newPos = mapToScene(center.x(), center.y());
    // 计算位置偏移，实现围绕中心点缩放的效果
    QPointF delta = oldPos - newPos;
    // 调整滚动条位置，使缩放中心保持不变
    horizontalScrollBar()->setValue(
    horizontalScrollBar()->value() + delta.x());
    verticalScrollBar()->setValue(
    verticalScrollBar()->value() + delta.y());
    m_wasFitToWindow = false;
}

// 适应窗口
void CustomGraphicsView::fitImageToWindow()
{
    if(m_originalPixmap.isNull())return;
    //计算缩放比比例
    qreal hScale = (qreal)viewport()->width() / m_originalPixmap.width();
    qreal vScale = (qreal)viewport()->height() / m_originalPixmap.height();
    qreal scale = qMin(hScale,vScale);
    zoomTo(scale);
    m_wasFitToWindow =true ;
}

// 实际大小
void CustomGraphicsView::actualSize()
{
    // 以视图中心为缩放中心
    QRect viewRect = viewport()->rect();
    QPointF center = QPointF(viewRect.width() / 2.0, viewRect.height() / 2.0);
    zoomTo(1.0, center);
}

// 放大
void CustomGraphicsView::zoomIn()
{
    zoomTo(m_scaleFactor*1.2);
}

// 缩小
void CustomGraphicsView::zoomOut()
{
    zoomTo(m_scaleFactor*0.8);
}

// 顺时针旋转90度
void CustomGraphicsView::rotateCW90()
{
    if (!m_pixmapItem) return;

    // 每次旋转前确保变换中心为项中心（项局部坐标）
    m_pixmapItem->setTransformOriginPoint(m_pixmapItem->boundingRect().center());

    // 顺时针旋转 90°
    m_pixmapItem->setRotation(m_pixmapItem->rotation() + 90.0);

    // 旋转后更新场景 rect（使用 sceneBoundingRect 获取旋转后的包围）
    m_scene->setSceneRect(m_pixmapItem->sceneBoundingRect());
}

// 完整的清空函数（重置所有状态 + 清理图片）
void CustomGraphicsView::clear()
{
    // 1. 先调用已有函数清理图片项
    clearCurrentImage();

    // 2. 清空原始像素图
    m_originalPixmap = QPixmap();

    // 3. 重置缩放因子
    m_scaleFactor = 1.0;

    // 4. 重置变换（清空缩放、旋转等所有变换）
    resetTransform();

    // 5. 重置适配窗口状态
    m_wasFitToWindow = false;

    // 6. 清空场景的包围矩形（回到初始空白）
    m_scene->setSceneRect(QRectF());

    // 7. 重置鼠标状态（防止清空后鼠标还处于拖拽状态）
    setCursor(Qt::ArrowCursor);
    setDragMode(NoDrag);
}











