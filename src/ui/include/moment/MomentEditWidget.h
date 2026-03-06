#ifndef MOMENTEDITWIDGET_H
#define MOMENTEDITWIDGET_H

#include <QWidget>
#include <QStringList>
#include <QLabel>
#include <functional>
// 补充必要头文件
#include <QFileInfo>
#include "FlowLayout.h"


// 定义图片/视频后缀
const QStringList IMAGE_SUFFIXES = { "jpg", "jpeg", "png", "bmp", "gif", "webp" };
const QStringList VIDEO_SUFFIXES = { "mp4", "avi", "mov", "mkv", "flv", "wmv" };

// 合并图片+视频过滤器，让用户可直接选择
const QString FILE_FILTER = "媒体文件 (*.jpg *.jpeg *.png *.bmp *.gif *.webp *.mp4 *.avi *.mov *.mkv *.flv *.wmv);;所有文件 (*.*)";

namespace Ui {
class MomentEditWidget;
}
class MediaDialog;
class ImgLabel;

// 文件类型枚举：区分当前选择状态
enum class FileType {
    None,   // 未选择任何文件
    Image,  // 选择了图片
    Video   // 选择了视频
};

class MomentEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MomentEditWidget(QWidget *parent = nullptr);
    ~MomentEditWidget();

    void setImages(const QStringList& imagePaths);
    void selectAndValidateFiles();
    bool pathIsEmpty(){return m_selectedFilePaths.isEmpty();}
    void setMediaDialog(MediaDialog* mediaDialog){m_mediaDialog=mediaDialog;}


signals:
    void momentPush(const QString &text, const QStringList & selectedFilePaths, FileType fileType);

private slots:
    void on_cancelButton_clicked();
    void on_confirmButton_clicked();
    void on_addButton_clicked();
    void onDeleteLabel(const QString& filePath);   // 处理删除

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MomentEditWidget *ui;
    QStringList m_selectedFilePaths;  // 选中的文件路径（图片/视频互斥）
    FlowLayout *m_flowLayout;         // 流式布局成员（操作布局必备）

    // 辅助函数：创建带样式的图片Label
    ImgLabel* createImageLabel(const QString& imagePath);
    // 辅助函数：创建视频占位Label（统一样式，方便后续替换为视频预览）
    ImgLabel* createVideoLabel(const QString& videoPath);
    // 辅助函数：显示文件校验错误弹窗
    void showFileValidateErrorDialog(const QString& errorMsg, std::function<void()> retryCallback);

    // 获取当前选中的文件类型（根据m_selectedFilePaths判断）
    FileType getCurrentFileType() const;
    // 刷新布局内容（渲染图片/视频 + 控制addButton显隐/位置）
    void refreshContentLayout();

    MediaDialog *m_mediaDialog;
};

#endif // MOMENTEDITWIDGET_H
