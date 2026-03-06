#include "MessageTextEdit.h"
#include <QMimeDatabase>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QTimer>
#include "FormatFileSize.h"
#include <QPainterPath>

MessageTextEdit::MessageTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setAcceptRichText(true);
    // 设置行高为固定值，避免图片高度影响行高计算

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

MessageTextEdit::~MessageTextEdit()
{
}


void MessageTextEdit::insertFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    FileItem item;
    item.filePath = filePath;
    item.fileName = fileInfo.fileName();

    // 判断是否为图片文件
    QMimeDatabase mimeDb;
    QString mimeType = mimeDb.mimeTypeForFile(filePath).name();
    item.isImage = mimeType.startsWith("image/");
    item.isVideo = mimeType.startsWith("video/");

    if (item.isImage) {
        QImage image(item.filePath);
        if (!image.isNull()) {
            item.thumbnail = createThumbnail(image);
        }
    } else {
        item.thumbnail = createFileIcon(item.filePath);
    }

    m_fileItems.append(item);
    insertFileToDocument(item);
}

void MessageTextEdit::insertFiles(const QStringList &filePaths)
{
    for (const QString &filePath : std::as_const(filePaths)) {
        insertFile(filePath);
    }
}

QList<FileItem> MessageTextEdit::getFileItems() const
{
    return m_fileItems;
}

void MessageTextEdit::clearContent()
{
    clear();
    m_fileItems.clear();
}

void MessageTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasUrls()) {

        QList<QUrl> urls = source->urls();
        QStringList filePaths;
        for (const QUrl &url : std::as_const(urls)) {
            if (url.isLocalFile()) {
                filePaths.append(url.toLocalFile());
            }
        }
        insertFiles(filePaths);
    } else {
        QTextEdit::insertFromMimeData(source);
    }
}

void MessageTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 检查是否按下了Shift或Ctrl键（Shift+Enter 或 Ctrl+Enter 换行）
        if ((event->modifiers() & Qt::ShiftModifier) ||
            (event->modifiers() & Qt::ControlModifier)) {
            QTextEdit::keyPressEvent(event);
        } else {
            emit returnPressed();
            event->accept();
        }
    } else {
        // 其他按键: 执行默认处理
        QTextEdit::keyPressEvent(event);
    }
}

QImage MessageTextEdit::createThumbnail(const QImage &image)
{
    return image.scaled(150,250, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage MessageTextEdit::createFileIcon(const QString &filePath)
{
    QImage icon(240,95, QImage::Format_ARGB32);
    icon.fill(Qt::transparent);

    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing);

    // 气泡参数设置
    const int margin = 3; // 透明边框边距
    const int bubblePadding = 12;
    const int iconWidth = 29;
    const int iconHeight = 40;

    // 计算实际气泡区域（减去边距）
    const int fileBubbleWidth = 240 - 2 * margin;
    const int fileBubbleHeight = 95 - 2 * margin;

    // 计算气泡区域（在透明边框内部）
    QRect fileBubbleRect(margin, margin, fileBubbleWidth, fileBubbleHeight);

    // 绘制文件气泡背景（白色圆角矩形）
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(fileBubbleRect, 5, 5);

    // 文件图标区域（右侧）
    QRect iconRect(fileBubbleRect.right() - bubblePadding - iconWidth,
                   fileBubbleRect.top() + bubblePadding,
                   iconWidth, iconHeight);

    // 绘制文件类型图标
    QFileInfo fileInfo(filePath);
    QString fileExtension = fileInfo.suffix ();
    paintFileIcon(&painter, iconRect, fileExtension);

    // 文本区域（左侧）
    QRect textRect(fileBubbleRect.left() + bubblePadding,
                   fileBubbleRect.top() + bubblePadding,
                   fileBubbleWidth - iconWidth - 3 * bubblePadding,
                   fileBubbleHeight - 2 * bubblePadding);

    // 绘制文件名
    painter.setPen(Qt::black);
    QFont fileNameFont = painter.font();
    fileNameFont.setPointSizeF(10.2);
    fileNameFont.setFamily("微软雅黑");
    painter.setFont(fileNameFont);

    // 文件名省略处理
    QString displayName = fileInfo.fileName();
    QFontMetrics nameMetrics(fileNameFont);
    if (nameMetrics.horizontalAdvance(fileInfo.fileName()) > textRect.width()) {
        displayName = nameMetrics.elidedText(fileInfo.fileName(), Qt::ElideRight, textRect.width());
    }
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, displayName);

    // 绘制文件大小
    QFont sizeFont = painter.font();
    sizeFont.setPointSizeF(9);
    painter.setFont(sizeFont);
    painter.setPen(QColor(150, 150, 150));

    QString fileSize;
    if (fileInfo.exists()) {
        qint64 sizeInBytes = fileInfo.size();
        fileSize = formatFileSize(sizeInBytes);
    }

    QRect sizeRect = textRect.adjusted(0, nameMetrics.height() + bubblePadding/2, 0, 0);
    painter.drawText(sizeRect, Qt::AlignLeft | Qt::AlignTop, fileSize);

    // 绘制底部横线和来源标识
    QFont sourceFont = painter.font();
    sourceFont.setPointSizeF(8.5);
    sourceFont.setFamily("微软雅黑");
    painter.setFont(sourceFont);
    painter.setPen(QColor(200, 200, 200));

    // 横线
    QLine dividerLine(fileBubbleRect.left() + bubblePadding, fileBubbleRect.bottom() - 25,
                      fileBubbleRect.right() - bubblePadding, fileBubbleRect.bottom() - 25);
    painter.drawLine(dividerLine);

    // 标识文字
    painter.setPen(QColor(150, 150, 150));
    painter.drawText(QRect(textRect.left(), fileBubbleRect.bottom() - 20,
                           textRect.width(), 15),
                     Qt::AlignLeft | Qt::AlignVCenter, "微信电脑版");

    return icon;
}

void MessageTextEdit::paintFileIcon(QPainter *painter, const QRect &fileRect, const QString &extension) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QString typeText;
    bool unknownType = false;
    // 设置图标颜色基于文件类型
    QColor iconColor;
    if (extension == "pdf") {
        iconColor = QColor(230, 67, 64);
        typeText = "PDF";
    } else if (extension == "doc" || extension == "docx") {
        iconColor = QColor(44, 86, 154);
        typeText = "W";
    } else if (extension == "xls" || extension == "xlsx") {
        iconColor = QColor(32, 115, 70);
        typeText = "X";
    } else if (extension == "ppt" || extension == "pptx") {
        iconColor = QColor(242, 97, 63);
        typeText = "P";
    } else if (extension == "txt") {
        iconColor = QColor(250, 157, 59);
        typeText = "txt";
    } else {
        iconColor = QColor(237, 237, 237);
        typeText = "*";
        unknownType = true;
    }

    // 计算缺角大小
    int foldSize = qMin(fileRect.width(), fileRect.height()) * 0.3;

    // 创建缺角矩形路径（右上角缺失）
    QPainterPath filePath;
    filePath.moveTo(fileRect.topLeft()); // A点：左上角
    filePath.lineTo(fileRect.topRight().x() - foldSize, fileRect.top()); // B点：右上角向左偏移
    filePath.lineTo(fileRect.topRight().x(), fileRect.top() + foldSize); // C点：右上角向下偏移
    filePath.lineTo(fileRect.bottomRight()); // D点：右下角
    filePath.lineTo(fileRect.bottomLeft()); // E点：左下角
    filePath.closeSubpath(); // 回到A点

    // 绘制文件主体（填充iconColor）
    painter->setPen(QPen(iconColor, 1));
    painter->setBrush(iconColor);
    painter->drawPath(filePath);

    // 绘制折角
    QColor foldedColor;
    if (unknownType) {
        foldedColor = iconColor.lighter(80);
    } else {
        foldedColor = iconColor.lighter(140);
    }
    painter->setPen(foldedColor);
    painter->setBrush(foldedColor);

    QPainterPath foldedPath;
    foldedPath.moveTo(filePath.elementAt(1));
    foldedPath.lineTo(filePath.elementAt(2));
    foldedPath.lineTo(filePath.elementAt(1).x, filePath.elementAt(2).y);
    foldedPath.closeSubpath();
    painter->drawPath(foldedPath);

    // 文件类型文字
    if (unknownType) {
        painter->setPen(QColor(86, 106, 148));
    } else {
        painter->setPen(Qt::white);
    }

    QFont iconFont = painter->font();

    // 根据文字长度调整字体大小
    if (typeText.length() <= 2) {
        iconFont.setPointSize(12);
    } else {
        iconFont.setPointSize(8);
    }

    QFontMetrics metrics(iconFont);
    painter->setFont(iconFont);

    // 调整文字区域
    QRect textRect = fileRect;
    textRect.setBottom(textRect.bottom() - 3);
    textRect.setTop(textRect.bottom() - metrics.height() - 3);

    painter->drawText(textRect, Qt::AlignHCenter, typeText);

    painter->restore();
}

void MessageTextEdit::insertFileToDocument(const FileItem &fileItem)
{
    QTextCursor cursor = textCursor();

    // 如果当前光标不在行首，先插入一个空格分隔
    if (!cursor.atBlockStart()) {
        cursor.insertText(" ");
    }

    if (fileItem.isImage) {
        // 插入图片
        QTextImageFormat imageFormat;
        imageFormat.setName(fileItem.filePath);
        imageFormat.setWidth(fileItem.thumbnail.width());
        imageFormat.setHeight(fileItem.thumbnail.height());

        // 将图片添加到文档资源
        document()->addResource(QTextDocument::ImageResource,
                                QUrl(fileItem.filePath),
                                QVariant(fileItem.thumbnail));

        cursor.insertImage(imageFormat);
    } else {
        // 插入文件图标
        QTextImageFormat iconFormat;
        iconFormat.setName("file_icon_" + fileItem.fileName);
        iconFormat.setWidth(fileItem.thumbnail.width());
        iconFormat.setHeight(fileItem.thumbnail.height());

        document()->addResource(QTextDocument::ImageResource,
                                QUrl("file_icon_" + fileItem.fileName),
                                QVariant(fileItem.thumbnail));

        cursor.insertImage(iconFormat);
    }

    setTextCursor(cursor);
}
