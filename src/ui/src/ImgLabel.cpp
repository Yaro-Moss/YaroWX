#include "ImgLabel.h"
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

ImgLabel::ImgLabel(QWidget *parent, int radius)
    : QLabel(parent), m_radius(radius)
{
    // 设置背景透明，避免圆角边缘出现默认背景色
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void ImgLabel::setRadius(int radius)
{
    m_radius = radius;
    update(); // 触发重绘
}

void ImgLabel::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    update(); // 触发重绘
}

void ImgLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // 发送点击信号，传递当前的pixmap
        emit labelClicked(m_pixmap);
    }
    // 调用父类方法，确保事件正常传递
    QLabel::mouseReleaseEvent(event);
}

void ImgLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (m_pixmap.isNull()) {
        // 如果没有图片，使用父类的默认绘制行为
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    // 开启抗锯齿，使圆角边缘更平滑
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 创建圆角路径作为裁剪区域
    QPainterPath path;
    path.addRoundedRect(rect(), m_radius, m_radius);
    painter.setClipPath(path);

    // 缩放图片以适应标签大小，保持比例
    QPixmap scaledPix = m_pixmap.scaled(
        size(),
        Qt::KeepAspectRatioByExpanding,  // 保持比例，允许超出边界（后续会被裁剪）
        Qt::SmoothTransformation        // 平滑缩放，避免模糊
        );

    // 计算图片居中显示的位置
    int x = (width() - scaledPix.width()) / 2;
    int y = (height() - scaledPix.height()) / 2;

    // 绘制图片
    painter.drawPixmap(x, y, scaledPix);
}
