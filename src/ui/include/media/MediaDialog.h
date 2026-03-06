#ifndef MEDIADIALOG_H
#define MEDIADIALOG_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QStackedWidget>
#include <QSplitter>

namespace Ui {
class MediaDialog;
}
class CustomGraphicsView;
class VideoPlayer;
class ThumbnailDelegate;
class ThumbnailPreviewModel;
class ThumbnailListView;
struct MediaItem;
class ThumbnailResourceManager;
enum class MediaType;


class MediaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MediaDialog(QWidget *parent = nullptr);

    ~MediaDialog();
    void playSinglePixmap(const QPixmap &pixmap);
    void playSingleVideo(QString path);
    void playNextMedia();                            // 播放下一个媒体
    void playPreviousMedia();                        // 播放上一个媒体
    void setMediaItems(const QList<MediaItem>& items);
    void selectMediaByMessageId(qint64 messageId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event)override;

private slots:
    void on_closeToolButton_clicked();
    void on_minimizeToolButton_clicked();
    void on_maximizeToolButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomOutButton_clicked();
    void on_actualSizeOrfitWindowButton_clicked();
    void on_rotateCW90ToolButton_clicked();
    void on_fixedToolButton_clicked();
    void on_nextButton_clicked();
    void on_prevButton_clicked();

    void on_thumbnailPreviewButton_clicked();
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

    void onMediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type);
private:
    Ui::MediaDialog *ui;
    QSplitter *splitter;
    QStackedWidget *mediaStackedWidget;
    void playMedia(const QString &path, const QString &mediaType);


    // 窗口相关
    bool m_isOnTop; // 记录当前是否置顶
    int m_titleBarHeight;
    bool m_isMaximized;
    bool m_isDragging;
    bool m_isDraggingMax;
    enum Edge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };
    Edge m_currentEdge;
    bool m_isResizing;
    int  m_borderWidth;
    QRect m_windowGeometry;       // 用于存储窗口几何信息
    QPoint m_dragStartPosition;   // 用于存储鼠标拖动起始位置

    // 图片相关
    QPixmap m_originalPixmap;
    qreal m_scaleFactor;
    bool m_wasFitToWindow;
    bool m_isDraggingImage;
    QPoint m_lastMousePos;
    CustomGraphicsView *m_graphicsView; // 已封装好场景等等 的视图

    ThumbnailResourceManager *thumbnailResourceManager;

    // 视频相关
    VideoPlayer * m_videoPlayer;

    // 缩略图预览列表模型
    ThumbnailListView * thumbnailView;
    ThumbnailDelegate *thumbnail_Delegate;
    ThumbnailPreviewModel *thumbnailPreview_Model;

    // 辅助函数
    Edge getEdge(const QPoint &pos);
    void updateCursorShape(const QPoint &pos);
    void titleBarSize();
    void mediaSize();

    QString currentImgPath;

};

#endif // MEDIADIALOG_H
