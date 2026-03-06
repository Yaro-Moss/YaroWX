#include "MomentWidgets.h"
#include "MomentEditWidget.h"
#include "ui_MomentMainWidget.h"
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <qevent.h>
#include <qpainter.h>


MomentMainWidget::MomentMainWidget(LocalMomentController* controller, QWidget *parent)
    : QWidget(parent),
    m_localMomentController(controller),
    ui(new Ui::MomentMainWidget),
    m_rightClickMenu(new QMenu(this))
    , m_weChatWidget(nullptr)
    , m_mediaDialog(nullptr)
    , m_contactController(nullptr)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);  // 开启鼠标追踪

    m_momentListLayout = new QVBoxLayout(ui->momentListWidget);
    m_momentListLayout->setContentsMargins(0, 0, 0, 0);
    m_momentListLayout->setSpacing(1);
    m_momentListLayout->setAlignment(Qt::AlignTop);


    // 滚动懒加载
    m_scrollArea = ui->scrollArea;
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MomentMainWidget::onScrollBarValueChanged);

    initializeMenu(); // 初始化右键菜单
    ui->addMomentButton->installEventFilter(this);    // 给addMomentButton安装事件过滤器

    // 初始加载朋友圈动态
    m_localMomentController->loadRecentMoments(5);
    connect(m_localMomentController, &LocalMomentController::momentsLoaded, this,
    [this](const QVector<LocalMoment>& moments, QHash<qint64,int> momentsIndexs,bool hasMore){
        m_moments = moments;
        m_momentsIndexs = momentsIndexs;
        m_hasMore = hasMore;
        showMoments();
    });
    connect(m_localMomentController, &LocalMomentController::momentUpdated, this,
    [this](const LocalMoment &moment){
        int index = m_momentsIndexs[moment.momentId];
        m_moments[index] = moment;
        if(!momentItemWidgets[index]){
            qDebug()<< "momentItemWidgets[index] == nullptr";
            return;
        }
        momentItemWidgets[index]->setLocalMoment(moment);
    });

    ui->coverLabel->setRadius(0);
}

MomentMainWidget::~MomentMainWidget()
{
    delete ui;
}

void MomentMainWidget::setMediaDialog(MediaDialog* mediaDialog)
{
    m_mediaDialog = mediaDialog;
    ui->coverLabel->setMediaDialog(m_mediaDialog);
    ui->avatarPushButton->setMediaDialog(m_mediaDialog);
}

void MomentMainWidget::setWeChatWidget(WeChatWidget* weChatWidget)
{
    m_weChatWidget = weChatWidget;
    ui->avatarPushButton->setWeChatWidget(m_weChatWidget);
}


void MomentMainWidget::setCurrentUser(const Contact &currentUser)
{
    m_currentUser = currentUser;
    ui->avatarPushButton->setContact(m_currentUser);
    ui->coverLabel->setPixmap(QPixmap(m_currentUser.user.profile_cover));
}


void MomentMainWidget::initializeMenu(){
    m_rightClickMenu->setFixedSize(140, 88);
    m_rightClickMenu->setStyleSheet(R"(
        QMenu {
            background-color: white;    /* 白色背景 */
            border-radius: 4px;         /* 圆角 */
            border: 1px solid #ccc;
            padding: 6px;               /* 去除内边距，避免尺寸偏差 */
        }
        QMenu::item {
            background-color: white;    /* 菜单项默认白色背景 */
            border: none;               /* 菜单项无边框 */
            padding: 4px 8px;           /* 菜单项内边距（上下4，左右8） */
            margin: 0px;                /* 菜单项外边距 */
            height: 29px;               /* 每个菜单项高度 */
            font: 15px "微软雅黑";

        }
        QMenu::item:selected {
            background-color: #07C160;  /* 微信绿色悬停背景 */
            color: white;               /* 悬停时文字白色 */
            border-radius: 4px;         /* 菜单项悬停圆角（可选） */
        }
    )");

    // 添加菜单项并绑定槽函数
    QAction* selectPhotoVideoAct = m_rightClickMenu->addAction("选择照片或视频");
    QAction* publishTextAct = m_rightClickMenu->addAction("发表文字");
    connect(selectPhotoVideoAct, &QAction::triggered, this, &MomentMainWidget::onSelectPhotoVideoTriggered);
    connect(publishTextAct, &QAction::triggered, this, &MomentMainWidget::onPublishTextTriggered);
}

void MomentMainWidget::onScrollBarValueChanged(int value)
{
    QScrollBar* scrollBar = m_scrollArea->verticalScrollBar();

    if (value >= (scrollBar->maximum() - 50) && m_hasMore) {
        m_localMomentController->loadMoreMoments(10);
    }
}

void MomentMainWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(140,140,140),1);
    painter.setPen(pen);
    painter.setBrush(QColor(238,238,238));

    QRect rect = QRect(2,2,width()-4,height()-4);
    painter.drawRoundedRect(rect, 8, 8);
}

// 鼠标按下：区分「顶部拉伸」「顶部拖动」「底部拉伸」
void MomentMainWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 1. 顶部边缘拉伸区（最顶部10px）
        if (event->pos().y() <= m_resizeMargin) {
            m_isResizingTop = true;
            m_dragStartPos = event->globalPosition().toPoint();  // 记录全局坐标（用于计算偏移）
            event->accept();
            return;
        }

        // 2. 顶部栏拖动区（拉伸区下方到50px）
        if (event->pos().y() > m_resizeMargin+6 && event->pos().y() <= m_topBarHeight) {
            m_isDragging = true;
            m_dragStartPos = event->pos();  // 记录相对窗口坐标（用于拖动）
            event->accept();
            return;
        }

        // 3. 底部边缘拉伸区（最底部10px）
        if (event->pos().y() >= height() - m_resizeMargin) {
            m_isResizingBottom = true;
            m_dragStartPos = event->globalPosition().toPoint();
            event->accept();
            return;
        }
    }

    QWidget::mousePressEvent(event);
}

// 鼠标移动：处理拉伸/拖动逻辑
void MomentMainWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 顶部拉伸逻辑（调整高度 + 窗口位置）
    if (m_isResizingTop) {
        // 计算鼠标y方向偏移
        int yDelta = event->globalPosition().toPoint().y() - m_dragStartPos.y();
        // 新高度 = 原高度 - 偏移（鼠标向下拖→高度增加，向上拖→高度减少）
        int newHeight = height() - yDelta;

        // 限制最小高度
        if (newHeight < m_minWindowHeight) {
            newHeight = m_minWindowHeight;
            yDelta = height() - newHeight; // 修正偏移，避免窗口跳动
        }

        // 调整窗口：y坐标随鼠标移动，高度同步变化
        move(x(), y() + yDelta);
        resize(width(), newHeight);
        m_dragStartPos = event->globalPosition().toPoint();  // 更新起始位置，避免累计误差
        event->accept();
        return;
    }

    // 底部拉伸逻辑（仅调整高度）
    if (m_isResizingBottom) {
        int yDelta = event->globalPosition().toPoint().y() - m_dragStartPos.y();
        int newHeight = height() + yDelta;

        if (newHeight < m_minWindowHeight) {
            newHeight = m_minWindowHeight;
        }

        resize(width(), newHeight);
        m_dragStartPos = event->globalPosition().toPoint();
        event->accept();
        return;
    }

    // 窗口拖动逻辑（和之前一致）
    if (m_isDragging) {
        QPoint delta = event->pos() - m_dragStartPos;
        move(pos() + delta);
        event->accept();
        return;
    }

    // 光标样式切换（顶部/底部拉伸区显示垂直拉伸光标）
    if (event->pos().y() <= m_resizeMargin || event->pos().y() >= height() - m_resizeMargin) {
        setCursor(Qt::SizeVerCursor);  // 垂直拉伸光标
    } else if (event->pos().y() <= m_topBarHeight) {
        setCursor(Qt::ArrowCursor);    // 顶部拖动区显示普通光标
    } else {
        setCursor(Qt::ArrowCursor);    // 中间区域普通光标
    }

    QWidget::mouseMoveEvent(event);
}

// 鼠标释放：重置所有状态
void MomentMainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_isResizingTop = false;
        m_isResizingBottom = false;
        setCursor(Qt::ArrowCursor);  // 恢复普通光标
        event->accept();
    }

    QWidget::mouseReleaseEvent(event);
}

void MomentMainWidget::on_minWinButton_clicked()
{
    showMinimized();
}

void MomentMainWidget::on_closeButton_clicked()
{
    close();
}

// 发朋友圈
void MomentMainWidget::on_addMomentButton_clicked()
{
    if(!m_momentEditWidget){
        m_momentEditWidget = new MomentEditWidget();
        m_momentEditWidget->setMediaDialog(m_mediaDialog);
        connect(m_momentEditWidget, &MomentEditWidget::momentPush, this, &MomentMainWidget::addMoment);
        m_momentEditWidget->selectAndValidateFiles();
    }

    if(m_momentEditWidget->pathIsEmpty()){
        m_momentEditWidget->close();
    }else{
        m_momentEditWidget->show();
    }
}

// 接收编辑好的朋友圈
void MomentMainWidget::addMoment(const QString &text, const QStringList & selectedFilePaths,
                                 FileType fileType)
{
    QString tx =text;
    QVector<MomentImageInfo> images;
    for (int i = 0; i < selectedFilePaths.size(); ++i) {
        MomentImageInfo img;
        img.localPath = selectedFilePaths[i]; // 本地路径
        img.downloadStatus = 2;               // 强制指定为「已下载」
        img.sort = i;                         // 按列表顺序排序
        images.append(img);
    }

    if(fileType==FileType::Video)
    {
        m_localMomentController->publishMoment(text, QVector<MomentImageInfo>(), images[0]);
    }else if(fileType==FileType::Image)
    {
        m_localMomentController->publishMoment(text, images);
    }else{
        m_localMomentController->publishMoment(text);
    }

    m_localMomentController->loadRecentMoments(5);
    m_momentEditWidget->close();
}


// 事件过滤器实现
bool MomentMainWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 1. 判断是否是addMomentButton的事件
    if (watched == ui->addMomentButton)
    {
        // 2. 捕获右键点击事件（QEvent::ContextMenu是标准右键菜单事件，也可以用QMouseEvent::RightButton）
        if (event->type() == QEvent::ContextMenu)
        {
            event->ignore();

            // 3. 在按钮右下角弹出自定义菜单
            QToolButton* btn = qobject_cast<QToolButton*>(watched);
            if (btn)
            {
                // 计算弹出位置：按钮的右下角（也可以用btn->mapToGlobal(QPoint(btn->width(), btn->height()))）
                QPoint popupPos = btn->mapToGlobal(QPoint(btn->width(), btn->height()));
                m_rightClickMenu->exec(popupPos);
            }

            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

// 菜单项槽函数实现
void MomentMainWidget::onSelectPhotoVideoTriggered()
{
    // 处理“选择照片或视频”的逻辑
    if(!m_momentEditWidget){
        m_momentEditWidget = new MomentEditWidget();
        m_momentEditWidget->setMediaDialog(m_mediaDialog);
        connect(m_momentEditWidget, &MomentEditWidget::momentPush, this, &MomentMainWidget::addMoment);
        m_momentEditWidget->selectAndValidateFiles();
    }

    if(m_momentEditWidget->pathIsEmpty()){
        m_momentEditWidget->close();
    }else{
        m_momentEditWidget->show();
    }
}

void MomentMainWidget::onPublishTextTriggered()
{
    if(!m_momentEditWidget){
        m_momentEditWidget = new MomentEditWidget();
        connect(m_momentEditWidget, &MomentEditWidget::momentPush, this, &MomentMainWidget::addMoment);
        m_momentEditWidget->setMediaDialog(m_mediaDialog);
    }
    m_momentEditWidget->show();
}

// 创建朋友圈项
MomentItemWidget* MomentMainWidget::createMomentItemWidget(const LocalMoment &localMoment)
{
    MomentItemWidget * momentItemWidget = new MomentItemWidget(ui->momentListWidget);
    momentItemWidget->setWeChatWidget(m_weChatWidget);
    momentItemWidget->setMediaDialog(m_mediaDialog);
    momentItemWidget->setContactController(m_contactController);
    momentItemWidget->setLocalMomentController(m_localMomentController);
    momentItemWidget->setCurrentUser(m_currentUser);
    momentItemWidget->setLocalMoment(localMoment);
    return momentItemWidget;
}

void MomentMainWidget::showMoments()
{
    while (QLayoutItem* item = m_momentListLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            delete w;
        }
        delete item;
    }
    momentItemWidgets.clear();
    for(const auto &moment : std::as_const(m_moments)){
        MomentItemWidget* momentItemWidget = createMomentItemWidget(moment);
        m_momentListLayout->addWidget(momentItemWidget);
        momentItemWidgets.append(momentItemWidget);
    }
}

void MomentMainWidget::on_refreshButton_clicked()
{
    m_localMomentController->loadRecentMoments(5);
}

