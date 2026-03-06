#include "MomentEditWidget.h"
#include "ClickClosePopup.h"
#include "ImgLabel.h"
#include "ui_MomentEditWidget.h"
#include <QLabel>
#include <QPixmap>
#include <QDebug>
#include <QLayoutItem>
#include <QFileDialog>
#include <QDir>
#include <qmenu.h>

MomentEditWidget::MomentEditWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MomentEditWidget)
    , m_selectedFilePaths(QStringList())
    , m_flowLayout(nullptr)
    , m_mediaDialog(nullptr)
{
    ui->setupUi(this);

    // 初始化流式布局
    if (ui->imgWidget->layout()) {
        QLayout* oldLayout = ui->imgWidget->layout();
        oldLayout->deleteLater();
    }
    Margins margins = Margins(0,0,0,0);
    m_flowLayout = new FlowLayout(nullptr, margins, 3, 3);
    ui->imgWidget->setLayout(m_flowLayout);

    setAttribute(Qt::WA_DeleteOnClose);
    ui->imgLabel1->deleteLater();
    // 初始刷新布局
    refreshContentLayout();
}

MomentEditWidget::~MomentEditWidget()
{
    delete ui;
}

// 辅助：获取当前选中的文件类型（图片/视频/无，互斥判断）
FileType MomentEditWidget::getCurrentFileType() const
{
    if (m_selectedFilePaths.isEmpty()) {
        return FileType::None;
    }
    // 因规则互斥，只需判断第一个文件的类型即可
    QFileInfo fileInfo(m_selectedFilePaths.first());
    QString suffix = fileInfo.suffix().toLower();
    if (IMAGE_SUFFIXES.contains(suffix)) {
        return FileType::Image;
    } else if (VIDEO_SUFFIXES.contains(suffix)) {
        return FileType::Video;
    }
    return FileType::None;
}

// 辅助：创建图片Label（保留原有逻辑，优化图片缩放）
ImgLabel* MomentEditWidget::createImageLabel(const QString& imagePath)
{
    ImgLabel* label = new ImgLabel(ui->imgWidget);
    label->setFixedSize(100, 100);
    label->setStyleSheet("border: 1px solid #ccc; border-radius: 4px; background: #f5f5f5;");
    label->setAlignment(Qt::AlignCenter);
    label->setScaledContents(false);
    label->setRadius(2);

    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        label->setPixmap(pixmap);
    } else {
        label->setText("图片\n加载失败");
        label->setStyleSheet(label->styleSheet() + "color: #999; font-size: 12px;");
        qDebug() << "图片加载失败：" << imagePath;
    }
    label->setMediaDialog(m_mediaDialog);

    label->setProperty("filePath", imagePath);
    label->installEventFilter(this);

    return label;
}

// 辅助：创建视频占位Label
ImgLabel* MomentEditWidget::createVideoLabel(const QString& videoPath)
{
    ImgLabel* label = new ImgLabel(ui->imgWidget);
    label->setFixedSize(160, 250);
    label->setStyleSheet("border: 1px solid #ccc; border-radius: 4px; background: #f5f5f5; color: #666; font-size: 12px;");
    label->setVideoPath(videoPath);
    label->setMediaDialog(m_mediaDialog);

    label->setProperty("filePath", videoPath);
    label->installEventFilter(this);
    return label;
}

// 刷新布局内容（渲染图片/视频 + 控制addButton显隐/位置）
void MomentEditWidget::refreshContentLayout()
{
    // 1. 清空原有布局内容（保留addButton，仅删除图片/视频Label）
    if (m_flowLayout) {
        while (m_flowLayout->count() > 0) {
            QLayoutItem* item = m_flowLayout->takeAt(0);
            if (item->widget() && item->widget() != ui->addButton) {
                item->widget()->deleteLater(); // 仅删除图片/视频Label
            }
            delete item;
        }
    }

    FileType curType = getCurrentFileType();
    // 2. 根据文件类型渲染图片/视频Label
    switch (curType) {
    case FileType::Image:
        // 遍历图片路径，添加图片Label
        for (const QString& path : m_selectedFilePaths) {
            m_flowLayout->addWidget(createImageLabel(path));
        }
        break;
    case FileType::Video:
        // 添加视频占位Label
        if (!m_selectedFilePaths.isEmpty()) {
            m_flowLayout->addWidget(createVideoLabel(m_selectedFilePaths.first()));
        }
        break;
    case FileType::None:
        // 未选择任何文件，无内容渲染
        break;
    }

    // 3. 控制addButton的显隐和位置（添加到布局最后）
    if (curType == FileType::Video) {
        // 视频模式：隐藏addButton
        ui->addButton->hide();
    } else if (curType == FileType::Image) {
        // 图片模式：不足9张显示，否则隐藏
        if (m_selectedFilePaths.count() < 9) {
            ui->addButton->show();
            m_flowLayout->addWidget(ui->addButton); // 显示在布局最后
        } else {
            ui->addButton->hide();
        }
    } else {
        // 无文件模式：显示addButton在最前
        ui->addButton->show();
        m_flowLayout->addWidget(ui->addButton);
    }

    // 刷新布局，确保样式生效
    m_flowLayout->update();
    ui->imgWidget->update();
}

// 文件选择+校验
void MomentEditWidget::selectAndValidateFiles()
{
    // 1. 打开文件选择对话框
    QStringList newFilePaths = QFileDialog::getOpenFileNames(
        this,
        tr("选择图片/视频文件"),
        QDir::homePath(),
        FILE_FILTER,
        nullptr
        );

    if (newFilePaths.isEmpty()) {
        return; // 用户取消选择，直接返回
    }

    // 2. 统计新选文件的图片/视频数量，过滤无效文件
    int newImageCount = 0;
    int newVideoCount = 0;
    QStringList validNewPaths; // 有效的新选文件路径
    for (const QString& path : newFilePaths) {
        QFileInfo fileInfo(path);
        QString suffix = fileInfo.suffix().toLower();
        if (IMAGE_SUFFIXES.contains(suffix)) {
            newImageCount++;
            validNewPaths << path;
        } else if (VIDEO_SUFFIXES.contains(suffix)) {
            newVideoCount++;
            validNewPaths << path;
        }
    }
    newFilePaths = validNewPaths;
    if (newFilePaths.isEmpty()) {
        showFileValidateErrorDialog(tr("请选择有效的图片或视频文件！"), [this]() {
            this->selectAndValidateFiles();
        });
        return;
    }

    // 3. 获取当前状态，结合已有文件做核心校验
    FileType curType = getCurrentFileType();
    int curFileCount = m_selectedFilePaths.count();
    QString errorMsg;

    // 校验规则1：新选文件不能同时包含图片和视频
    if (newImageCount > 0 && newVideoCount > 0) {
        errorMsg = tr("只能选择图片或视频中的一种类型！");
    }
    // 校验规则2：已有视频，禁止再选任何文件
    else if (curType == FileType::Video) {
        errorMsg = tr("已选择视频，无法再添加其他文件！");
    }
    // 校验规则3：已有图片，新选必须是图片且总数≤9
    else if (curType == FileType::Image) {
        if (newVideoCount > 0) {
            errorMsg = tr("已选择图片，无法添加视频！");
        } else if (curFileCount + newImageCount > 9) {
            errorMsg = tr("图片最多只能选择9张，还可选择%1张！").arg(9 - curFileCount);
        }
    }
    // 校验规则4：未选择文件，新选视频不能超过1个
    else if (curType == FileType::None) {
        if (newVideoCount > 1) {
            errorMsg = tr("视频最多只能选择1个！");
        } else if (newImageCount > 9) {
            errorMsg = tr("图片最多只能选择9张！");
        }
    }

    // 校验失败：显示弹窗并支持重新选择
    if (!errorMsg.isEmpty()) {
        showFileValidateErrorDialog(errorMsg, [this]() {
            this->selectAndValidateFiles();
        });
        return;
    }

    // 校验成功：追加有效文件到成员变量，刷新布局
    m_selectedFilePaths << newFilePaths;
    // 图片模式去重
    if (curType == FileType::Image || newImageCount > 0) {
        m_selectedFilePaths.removeDuplicates();
    }
    refreshContentLayout();
}

// addButton点击（直接调用文件选择校验）
void MomentEditWidget::on_addButton_clicked()
{
    selectAndValidateFiles();
}

// 设置图片列表（外部调用时刷新布局）
void MomentEditWidget::setImages(const QStringList& imagePaths)
{
    m_selectedFilePaths = imagePaths;
    refreshContentLayout();
}

// 取消按钮
void MomentEditWidget::on_cancelButton_clicked()
{
    close();
}

// 确认按钮（发射信号，传递文本和选中的文件路径）
void MomentEditWidget::on_confirmButton_clicked()
{
    QString text = ui->textEdit->toPlainText().trimmed();
    emit momentPush(text, m_selectedFilePaths, getCurrentFileType());
    close();
}

// 自定义错误弹窗
void MomentEditWidget::showFileValidateErrorDialog(const QString& errorMsg, std::function<void()> retryCallback)
{
    ClickClosePopup* errorDialog = new ClickClosePopup(this);
    errorDialog->setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(errorDialog);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    QLabel* messageLabel = new QLabel(errorMsg);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setStyleSheet(R"(
        font-family: 'Microsoft YaHei';
        font-size: 14px;
        color: #333333;
        padding: 10px;
        border: none;
        background-color: transparent;
    )");

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* retryButton = new QPushButton(tr("重新选择文件"));
    QPushButton* cancelButton = new QPushButton(tr("取消"));

    retryButton->setStyleSheet(R"(
        QPushButton {
            background-color: rgb(7, 193, 96);
            color: white;
            border: none;
            border-radius: 4px;
            min-width: 80px;
            min-height: 30px;
            font-family: "Microsoft YaHei";
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: rgb(7, 182, 88);
        }
        QPushButton:pressed {
            background-color: rgb(6, 178, 83);
        }
    )");

    cancelButton->setStyleSheet(R"(
        QPushButton {
            background-color: #f0f0f0;
            color: #333333;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            min-width: 80px;
            min-height: 30px;
            font-family: "Microsoft YaHei";
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #e0e0e0;
        }
        QPushButton:pressed {
            background-color: #d0d0d0;
        }
    )");

    buttonLayout->addStretch();
    buttonLayout->addWidget(retryButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    mainLayout->addWidget(messageLabel);
    mainLayout->addLayout(buttonLayout);

    connect(retryButton, &QPushButton::clicked, errorDialog, [errorDialog, retryCallback]() {
        errorDialog->close();
        if (retryCallback) retryCallback();
    });
    connect(cancelButton, &QPushButton::clicked, errorDialog, &ClickClosePopup::close);

    errorDialog->adjustSize();
    errorDialog->setFixedSize(errorDialog->width(), errorDialog->height());
    errorDialog->exec();
}

bool MomentEditWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ContextMenu) {
        // 只处理 ImgLabel 的右键事件（自动排除加号按钮）
        ImgLabel *label = qobject_cast<ImgLabel*>(obj);
        if (label) {
            QContextMenuEvent *contextEvent = static_cast<QContextMenuEvent*>(event);
            QMenu menu(this);

            // 设置菜单的样式表，实现悬停红色效果
            menu.setStyleSheet(R"(
                QMenu {
                    background-color: white;  /* 菜单背景色 */
                    border: 1px solid #cccccc; /* 菜单边框 */
                }
                QMenu::item {
                    padding: 5px 20px;  /* 菜单项内边距，优化点击区域 */
                    color: black;       /* 正常状态文字颜色 */
                }
                QMenu::item:selected {  /* 悬停/选中状态 */
                    background-color: red;  /* 悬停背景红色 */
                    color: white;           /* 悬停文字白色（对比更清晰） */
                }
                QMenu::item:disabled {
                    color: #aaaaaa;  /* 禁用状态文字颜色 */
                }
            )");

            QAction *deleteAction = menu.addAction(tr("删除"));

            // 获取之前保存的文件路径
            QString filePath = label->property("filePath").toString();
            connect(deleteAction, &QAction::triggered, this, [this, filePath]() {
                onDeleteLabel(filePath);
            });

            menu.exec(contextEvent->globalPos());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MomentEditWidget::onDeleteLabel(const QString& filePath)
{
    m_selectedFilePaths.removeOne(filePath);
    // 刷新布局，重新生成所有标签（包括加号按钮）
    refreshContentLayout();
}
