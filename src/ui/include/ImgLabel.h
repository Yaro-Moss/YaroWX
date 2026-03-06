#ifndef IMGLABEL_H
#define IMGLABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
class MediaDialog;

class ImgLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ImgLabel(QWidget *parent = nullptr, int radius = 6);

    void setRadius(int radius);
    void setPixmap(const QPixmap &pixmap);
    void setMediaDialog(MediaDialog* mediaDialog){m_mediaDialog = mediaDialog;}
    void setVideoPath(QString path);

signals:
    void labelClicked(const QPixmap &pixmap);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius;         // 圆角半径
    QPixmap m_pixmap;     // 存储要显示的图片
    MediaDialog *m_mediaDialog;

    QString videoPath = QString();

};

#endif // IMGLABEL_H
