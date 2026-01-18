#include "MediaDialog.h"
#include "ui_MediaDialog.h"
#include "CustomGraphicsView.h"
#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QtMath>
#include <QScrollBar>
#include "VideoPlayer.h"
#include <QFileInfo>
#include "ThumbnailDelegate.h"
#include "ThumbnailPreviewModel.h"
#include "ThumbnailListView.h"
#include "ThumbnailResourceManager.h"

MediaDialog::MediaDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MediaDialog)
    // 窗口相关
    , m_isOnTop(false)
    , m_titleBarHeight(40)
    , m_isMaximized(false)
    , m_isDragging(false)
    , m_isDraggingMax(false)
    , m_currentEdge(None)
    , m_isResizing(false)
    , m_borderWidth(8)
    , thumbnailResourceManager(new ThumbnailResourceManager(this))
    // 媒体

{
    ui->setupUi(this);


    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);   // 设置鼠标跟踪

    qApp->installEventFilter(this);
    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setContentsMargins(0, 0, 0, 0);
    splitter->setHandleWidth(0);

    mediaStackedWidget = new QStackedWidget(splitter);
    m_graphicsView =  new CustomGraphicsView(mediaStackedWidget);
    m_videoPlayer = new VideoPlayer(mediaStackedWidget);
    mediaStackedWidget->addWidget(m_graphicsView);
    mediaStackedWidget->addWidget(m_videoPlayer);

    thumbnailView = new ThumbnailListView(splitter);
    thumbnail_Delegate = new ThumbnailDelegate(thumbnailView);
    thumbnailPreview_Model = new ThumbnailPreviewModel(thumbnailView);

    thumbnailView->setModel(thumbnailPreview_Model);
    thumbnailView->setItemDelegate(thumbnail_Delegate);

    thumbnailView->setViewMode(QListView::IconMode);  // 设置为图标模式（适合显示缩略图）
    thumbnailView->setFlow(QListView::LeftToRight);   // 项排列方向：从左到右（行优先）
    thumbnailView->setResizeMode(QListView::Adjust);  // 视图大小改变时自动调整布局
    thumbnailView->setWrapping(true);   // 允许自动换行（配合排列方向形成网格）
    thumbnailView->setSpacing(2);       // 项之间的间距设为2像素
    thumbnailView->setFixedWidth(230);  // 固定宽度230像素（控制每行显示3列）

    on_thumbnailPreviewButton_clicked();

    // 选中项改变
    connect(thumbnailView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MediaDialog::onCurrentChanged);


    titleBarSize(); // 动态设置标题栏位置大小
    mediaSize(); // 动态设置媒体区域大小

    connect(thumbnailResourceManager, &ThumbnailResourceManager::mediaLoaded, this, &MediaDialog::onMediaLoaded);

}


MediaDialog::~MediaDialog()
{
    delete ui;
    qApp->removeEventFilter(this);
}


// 绘制边框-------------------------------------------
void MediaDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(170,170,170),1);
    painter.setPen(pen);
    painter.setBrush(QColor(252,252,252));
    QRect rect = QRect(1,1,width()-2,height()-2);

    painter.drawRoundedRect(rect,m_isMaximized? 0:8,m_isMaximized? 0:8);
}


// 判断鼠标位置是否在窗口边缘------------------------------------------
MediaDialog::Edge MediaDialog::getEdge(const QPoint &pos)
{
    // 获取窗口矩形
    QRect rect = this->rect();

    if (pos.x() <= m_borderWidth && pos.y() <= m_borderWidth)
        return TopLeft;

    if (pos.x() >= rect.width() - m_borderWidth && pos.y() <= m_borderWidth)
        return TopRight;

    if (pos.x() <= m_borderWidth && pos.y() >= rect.height() - m_borderWidth)
        return BottomLeft;

    if (pos.x() >= rect.width() - m_borderWidth && pos.y() >= rect.height() - m_borderWidth)
        return BottomRight;

    if (pos.x() <= m_borderWidth)
        return Left;

    if (pos.x() >= rect.width() - m_borderWidth)
        return Right;

    if (pos.y() <= m_borderWidth)
        return Top;

    if (pos.y() >= rect.height() - m_borderWidth)
        return Bottom;

    return None;
}


// 更新鼠标样式------------------------------------------------
void MediaDialog::updateCursorShape(const QPoint &pos)
{
    Edge edge = getEdge(pos);

    switch (edge) {
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case TopLeft:
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRight:
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}


// 窗口鼠标按下事件--------------------------------------------
void MediaDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_currentEdge = getEdge(event->pos());
        // 如果在边缘，开始调整大小
        if (m_currentEdge != None) {
            m_isResizing = true;
            m_windowGeometry = geometry();
            event->accept();
        }
        // 否则，准备移动窗口 // 标题栏拖动
        else if (event->pos().y() <= m_titleBarHeight) {
            if(m_isMaximized) m_isDraggingMax = true;
            m_dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_isDragging = true;
            event->accept();
        }
    }
    QDialog::mousePressEvent(event);
}


// 窗口鼠标移动事件---------------------------------------------
void MediaDialog::mouseMoveEvent(QMouseEvent *event)
{
    updateCursorShape(event->pos());
    // 如果正在调整大小
    if (m_isResizing && m_currentEdge != None && !m_isMaximized) {
        QPoint currentPos = event->globalPosition().toPoint();
        QRect newGeometry = m_windowGeometry;
        // 根据边缘类型调整窗口大小
        switch (m_currentEdge) {
        case Left:
            newGeometry.setLeft(qMin(currentPos.x(), newGeometry.right() - minimumWidth()));
            break;
        case Right:
            newGeometry.setRight(qMax(currentPos.x(), newGeometry.left() + minimumWidth()));
            break;
        case Top:
            newGeometry.setTop(qMin(currentPos.y(), newGeometry.bottom() - minimumHeight()));
            break;
        case Bottom:
            newGeometry.setBottom(qMax(currentPos.y(), newGeometry.top() + minimumHeight()));
            break;
        case TopLeft:
            newGeometry.setLeft(qMin(currentPos.x(), newGeometry.right() - minimumWidth()));
            newGeometry.setTop(qMin(currentPos.y(), newGeometry.bottom() - minimumHeight()));
            break;
        case TopRight:
            newGeometry.setRight(qMax(currentPos.x(), newGeometry.left() + minimumWidth()));
            newGeometry.setTop(qMin(currentPos.y(), newGeometry.bottom() - minimumHeight()));
            break;
        case BottomLeft:
            newGeometry.setLeft(qMin(currentPos.x(), newGeometry.right() - minimumWidth()));
            newGeometry.setBottom(qMax(currentPos.y(), newGeometry.top() + minimumHeight()));
            break;
        case BottomRight:
            newGeometry.setRight(qMax(currentPos.x(), newGeometry.left() + minimumWidth()));
            newGeometry.setBottom(qMax(currentPos.y(), newGeometry.top() + minimumHeight()));
            break;
        default:
            break;
        }
        setGeometry(newGeometry);
        event->accept();
    }
    // 如果正在移动窗口
    else if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        if(m_isDraggingMax){//如果在窗口最大化时拖动
            on_maximizeToolButton_clicked();
            QPoint globalMousePos = event->globalPosition().toPoint();  // 获取鼠标全局位置
            QScreen *screen = QGuiApplication::primaryScreen();        // 获取屏幕信息（主屏幕）
            QRect screenGeometry = screen->geometry();
            int screenWidth = screenGeometry.width();
            int screenHeight = screenGeometry.height();

            double horizontalRatio = static_cast<double>(globalMousePos.x()) / screenWidth;
            int mouseXInWindow = static_cast<int>(horizontalRatio *  width());
            int mouseYInWindow = m_titleBarHeight / 2;

            // 计算窗口新位置（窗口左上角坐标）
            int newWindowX = globalMousePos.x() - mouseXInWindow;
            int newWindowY = globalMousePos.y() - mouseYInWindow;
            newWindowX = qMax(0, qMin(newWindowX, screenWidth -  width()));
            newWindowY = qMax(0, qMin(newWindowY, screenHeight - height()));

            // 移动窗口到新位置
            move(newWindowX, newWindowY);
            m_dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_isDraggingMax = false; // 使前面if代码块一次只触发一次，防止拖动过程一直触发
        }
        move(event->globalPosition().toPoint() - m_dragStartPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}


// 窗口鼠标释放事件-------------------------------
void MediaDialog::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDragging = false;
    m_isDraggingMax = false;
    m_isResizing = false;
    m_currentEdge = None;
    setCursor(Qt::ArrowCursor);
    event->accept();
    QDialog::mouseReleaseEvent(event);
}

bool MediaDialog::eventFilter(QObject *watched, QEvent *event) {
    // 筛选「鼠标移动事件」（QApplication的事件）更新光标
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos = mouseEvent->globalPosition().toPoint();//全局坐标
        if (this->geometry().contains(this->mapFromGlobal(globalPos))) {
            QPoint windowLocalPos = this->mapFromGlobal(globalPos);
            this->updateCursorShape(windowLocalPos);
        }
        else {
            this->setCursor(Qt::ArrowCursor);
        }
    }
    return false;
}

// 放大按钮事件处理
void MediaDialog::on_zoomInButton_clicked()
{
    m_graphicsView->zoomIn();
}

// 缩小按钮事件处理
void MediaDialog::on_zoomOutButton_clicked()
{
    m_graphicsView->zoomOut();
}

//关闭
void MediaDialog::on_closeToolButton_clicked()
{
    close();
}

//最小化
void MediaDialog::on_minimizeToolButton_clicked()
{
    showMinimized();
}

//最大化与还原
void MediaDialog::on_maximizeToolButton_clicked()
{
    if (!m_isMaximized) {
        //最大化
        showMaximized();
        m_isMaximized = true;
        QToolButton* toolBtn = ui->maximizeToolButton;
        if (toolBtn) {
            toolBtn->setIcon(QIcon(":/a/icons/还原.svg"));
        }
    } else {//还原
        showNormal();
        m_isMaximized = false;
        QToolButton* toolBtn = ui->maximizeToolButton;
        if (toolBtn) {
            toolBtn->setIcon(QIcon(":/a/icons/窗口最大化.svg"));
        }
    }
}


//标题栏刷新位置和大小
void MediaDialog::titleBarSize()
{
    QWidget *titleBar = ui->titleBar;
    titleBar->setGeometry(1,1,width()-2,m_titleBarHeight);
}


// 媒体区域占据标题栏以外的空间
void MediaDialog::mediaSize()
{
    splitter->setGeometry(3, 40, width()-6, height()-m_titleBarHeight-4);
}


//窗口大小变化事件处理
void MediaDialog::resizeEvent(QResizeEvent *event)
{
    titleBarSize() ;// 刷新标题栏
    mediaSize(); //刷新视图大小
    QDialog::resizeEvent(event);
}


void MediaDialog::on_actualSizeOrfitWindowButton_clicked()
{
    if(!m_wasFitToWindow)
    {
        m_graphicsView->fitImageToWindow();
        QToolButton *button = ui->actualSizeOrfitWindowButton;
        button->setIcon(QIcon(":/a/icons/图片原始大小.svg"));
        button->setToolTip("图片原始大小");
        m_wasFitToWindow = true;

    }else{
        m_graphicsView->actualSize();
        QToolButton *button = ui->actualSizeOrfitWindowButton;
        button->setIcon(QIcon(":/a/icons/适应窗口大小.svg"));
        button->setToolTip("图片适应窗口");
        m_wasFitToWindow = false;
    }
}


void MediaDialog::on_rotateCW90ToolButton_clicked()
{
    m_graphicsView->rotateCW90();

}


void MediaDialog::on_fixedToolButton_clicked()
{
    // 切换置顶状态
    m_isOnTop = !m_isOnTop;

    // 更新窗口标志
    if (m_isOnTop) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        ui->fixedToolButton->setToolTip("取消置顶");
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
        ui->fixedToolButton->setToolTip("置顶");
    }
    // 重新显示窗口（设置窗口标志后需要重新显示才能生效）
    show();
}


void MediaDialog::playSinglePixmap(const QPixmap &pixmap)
{
    mediaStackedWidget->setCurrentWidget(m_graphicsView);
    m_graphicsView->loadImage(pixmap);
    thumbnailPreview_Model->clearAllMediaItems();
}

void MediaDialog::playMedia(const QString &path, const QString &mediaTytpe)
{
    if(mediaTytpe=="video"){
        m_videoPlayer->loadVideo(path);
        mediaStackedWidget->setCurrentWidget(m_videoPlayer);
        currentImgPath = QString();
    }else if (mediaTytpe=="image") {
        m_videoPlayer->stop();
        currentImgPath = path;
        QPixmap img = thumbnailResourceManager->getThumbnail(path, QSize(), MediaType::OriginalImage);
        m_graphicsView->loadImage(img);
        mediaStackedWidget->setCurrentWidget(m_graphicsView);
    }
}


// 播放下一个媒体
void MediaDialog::playNextMedia()
{
    if (!thumbnailView || !thumbnailPreview_Model) return;

    QModelIndex currentIndex = thumbnailView->currentIndex();
    int rowCount = thumbnailPreview_Model->rowCount();

    int nextRow = 0;
    if (currentIndex.isValid()) {
        nextRow = currentIndex.row() + 1;
        if (nextRow >= rowCount) {
            nextRow = 0;
        }
    }

    QModelIndex nextIndex = thumbnailPreview_Model->index(nextRow, 0);
    if (nextIndex.isValid()) {
        thumbnailView->setCurrentIndex(nextIndex);
    }
}


// 播放上一个媒体
void MediaDialog::playPreviousMedia()
{
    if(!thumbnailView || !thumbnailPreview_Model) return;

    QModelIndex currentIndex = thumbnailView->currentIndex();
    int rowCount = thumbnailPreview_Model->rowCount();

    int previousRow = 0;
    if(currentIndex.isValid()){
        previousRow = currentIndex.row() - 1;
        if(previousRow < 0){
            previousRow = rowCount - 1;
        }
    }

    QModelIndex previousIndex = thumbnailPreview_Model->index(previousRow,0);
    if (previousIndex.isValid()) {
        thumbnailView->setCurrentIndex(previousIndex);
    }
}

// 下一个媒体按钮点击事件
void MediaDialog::on_nextButton_clicked()
{
    playNextMedia();
}

// 上一个媒体按钮点击事件
void MediaDialog::on_prevButton_clicked()
{
    playPreviousMedia();
}

void MediaDialog::on_thumbnailPreviewButton_clicked()
{
    const bool isHidden = thumbnailView->isHidden();
    thumbnailView->setHidden(!isHidden);
}

void MediaDialog::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if(current.isValid()){

        QString thumbPath = current.data(ThumbnailPreviewModel::ThumbnailPathRole).toString();
        QString sourcePath = current.data(ThumbnailPreviewModel::SourceMediaPathRole).toString();
        QString mediaType = current.data(ThumbnailPreviewModel::MediaTypeRole).toString();

        QFileInfo fileInfo(sourcePath);
        if(fileInfo.exists() && fileInfo.isFile()){
            playMedia(sourcePath, mediaType);
        }else{
            m_videoPlayer->stop();
            QPixmap warningPix = ThumbnailResourceManager::getWarningThumbnail(thumbPath, mediaType);
            mediaStackedWidget->setCurrentWidget(m_graphicsView);
            m_graphicsView->loadImage(warningPix);
        }
    }
}

void MediaDialog::setMediaItems(const QList<MediaItem>& items)
{
    thumbnailPreview_Model->setMediaItems(items);
}

void MediaDialog::selectMediaByMessageId(qint64 messageId)
{
    if (!thumbnailPreview_Model) {
        qWarning() << "Thumbnail model is not set";
        return;
    }

    QModelIndex index = thumbnailPreview_Model->indexFromMessageId(messageId);
    if (index.isValid()) {

        thumbnailView->setCurrentIndex(index);
        thumbnailView->scrollTo(index, QAbstractItemView::PositionAtCenter);

    } else {
        qWarning() << "Media item with messageId" << messageId << "not found";
    }
}


void MediaDialog::onMediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type)
{
    if(currentImgPath == resourcePath && type==MediaType::OriginalImage){
        m_graphicsView->loadImage(media);
        mediaStackedWidget->setCurrentWidget(m_graphicsView);
    }
}


