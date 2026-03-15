#ifndef MESSAGETEXTEDIT_H
#define MESSAGETEXTEDIT_H

#include <QTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextImageFormat>
#include <QMimeData>
#include <QImage>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QDebug>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

struct FileItem {
    QString filePath;
    QString fileName;
    bool isImage = false;
    bool isVideo = false;
    QImage thumbnail; // 用于图片的缩略图
};

class MessageTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit MessageTextEdit(QWidget *parent = nullptr);
    ~MessageTextEdit();
    // 插入文件
    void insertFile(const QString &filePath);
    // 插入多个文件
    void insertFiles(const QStringList &filePaths);
    // 获取所有文件项
    QList<FileItem> getFileItems() const;
    // 清空所有内容
    void clearContent();


signals:
    void returnPressed();


protected:
    // 重写粘贴事件
    void insertFromMimeData(const QMimeData *source) override;
    void keyPressEvent(QKeyEvent *event) override;  // 重写键盘事件处理,回车发送


private:
    // 文件项列表
    QList<FileItem> m_fileItems;

    // 创建图片缩略图
    QImage createThumbnail(const QImage &image);

    // 创建文件图标
    QImage createFileIcon(const QString &filePath);
    void paintFileIcon(QPainter *painter, const QRect &fileRect, const QString &extension) const;


    // 插入文件到文档
    void insertFileToDocument(const FileItem &fileItem);



};

#endif // MESSAGETEXTEDIT_H
