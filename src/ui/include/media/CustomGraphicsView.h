#ifndef CUSTOMGRAPHICSVIEW_H
#define CUSTOMGRAPHICSVIEW_H
#include <QGraphicsView>

class CustomGraphicsView: public QGraphicsView
{
    Q_OBJECT
public:
    explicit CustomGraphicsView(QWidget *parent = nullptr);
    void fitImageToWindow();
    void loadImage(const QString &imagePath);
    void loadImage(const QPixmap &pixmap);
    void actualSize();
    void zoomIn();
    void zoomOut();
    void rotateCW90();
    void clear();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event)override;

private:
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    QPixmap m_originalPixmap;

    qreal m_scaleFactor;
    bool m_wasFitToWindow;

    void zoomTo(qreal scale, const QPointF &center = QPointF());
    void clearCurrentImage();
};

#endif // CUSTOMGRAPHICSVIEW_H
