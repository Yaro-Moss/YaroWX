#include "ChatMessageDelegate.h"
#include "FormatTime.h"
#include <QFontMetrics>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QPainterPath>
#include <QAbstractItemView>
#include "ChatMessagesModel.h"
#include "ChatMessageListView.h"
#include "AudioPlayer.h"

ChatMessageDelegate::ChatMessageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    thumbnailManager = ThumbnailResourceManager::instance();
    connect(thumbnailManager, &ThumbnailResourceManager::mediaLoaded,
            this, &ChatMessageDelegate::onMediaLoaded);
}

void ChatMessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    if (!index.isValid()) return;

    // 保存painter状态
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制选中背景
    if (option.state & QStyle::State_Selected) {
        QColor selectedColor = QColor(220, 220, 220);
        painter->fillRect(option.rect, selectedColor);
    }

    Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
    bool isOwn = index.data(ChatMessagesModel::IsOwnRole).toBool();

    // 根据消息类型调用不同的绘制方法
    switch (message.type) {
    case MessageType::TEXT:
        paintTextMessage(painter, option, message, isOwn);
        break;
    case MessageType::IMAGE:
        paintImageMessage(painter, option, message, isOwn);
        break;
    case MessageType::VIDEO:
        paintVideoMessage(painter, option, message, isOwn);
        break;
    case MessageType::FILE:
        paintFileMessage(painter, option, message ,isOwn);
        break;
    case MessageType::VOICE:
        paintVoiceMessage(painter, option, message, isOwn);
        break;
    }

    painter->restore();
}

QSize ChatMessageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    if (!index.isValid()) return QSize(100, 30);

    Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
    int width = option.rect.width();

    const int margin = MARGIN;
    const int bubblePadding = BUBBLE_PADDING;
    const int avatarSize = AVATAR_SIZE;

    // 计算时间戳高度
    QFont timeFont = option.font;
    timeFont.setPointSizeF(7.5);
    QFontMetrics timeMetrics(timeFont);
    int timeHeight = timeMetrics.height();

    switch (message.type) {
    case MessageType::TEXT: {
        // 计算文本内容所需尺寸
        int maxBubbleWidth = width * 0.6;
        QFont font = option.font;
        font.setPointSizeF(10.5);
        font.setFamily(QStringLiteral("微软雅黑"));
        QSize textSize = calculateTextSize(message.content, font, maxBubbleWidth - 2 * bubblePadding);
        int bubbleHeight = textSize.height() + 2 * bubblePadding;
        int avatarAreaHeight = avatarSize + 2 * margin;
        int contentAreaHeight = bubbleHeight + timeHeight + 2 * margin;
        return QSize(width, qMax(avatarAreaHeight, contentAreaHeight));
    }
    case MessageType::IMAGE: {
        QPixmap thumbnail = thumbnailManager->getThumbnail(message.filePath, QSize(200, 300),
                                             MediaType::ImageThumb,0, message.thumbnailPath);
        return QSize(width, thumbnail.height() + timeHeight + 2 * margin);
    }
    case MessageType::VIDEO: {
        QPixmap thumbnail = thumbnailManager->getThumbnail(message.filePath, QSize(200, 300),
                                             MediaType::ImageThumb,0,message.thumbnailPath);

        return QSize(width, thumbnail.height() + timeHeight + 2 * margin);
    }
    case MessageType::FILE: {
        const int fileBubbleHeight = FILE_BUBBLE_HEIGHT;
        int totalHeight = fileBubbleHeight + timeHeight + 2 * margin;
        int avatarAreaHeight = avatarSize + 2 * margin;
        return QSize(width, qMax(totalHeight, avatarAreaHeight));
    }
    case MessageType::VOICE: {
        const int voiceBubbleHeight = VOICE_BUBBLE_HEIGHT;
        int totalHeight = voiceBubbleHeight + timeHeight + 2 * margin;
        int avatarAreaHeight = avatarSize + 2 * margin;
        return QSize(width, qMax(totalHeight, avatarAreaHeight));
    }
    default:
        return QSize(100, 30 + 2 * margin);
    }
}

bool ChatMessageDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                      const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        // 只处理右键点击
        if (mouseEvent->button() == Qt::RightButton) {
            Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
            bool isOwn = index.data(ChatMessagesModel::IsOwnRole).toBool();

            QRect clickableRect = getClickableRect(option, message, isOwn);
            if (clickableRect.contains(mouseEvent->pos())) {
                QPoint globalPos = option.widget->mapToGlobal(mouseEvent->pos());
                emit rightClicked(globalPos, message);
                return true;
            }
        }
        // 左键点击处理
        else if (mouseEvent->button() == Qt::LeftButton) {
            return handleLeftClick(mouseEvent, option, index);
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

bool ChatMessageDelegate::handleLeftClick(QMouseEvent *mouseEvent, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
    bool isOwn = index.data(ChatMessagesModel::IsOwnRole).toBool();

    QRect clickableRect = getClickableRect(option, message, isOwn);
    if (!clickableRect.contains(mouseEvent->pos())) {
        return false;
    }

    switch (message.type) {
    case MessageType::IMAGE:
    case MessageType::VIDEO:
        emit mediaClicked(message.messageId, message.conversationId);
        break;
    case MessageType::FILE:
        emit fileClicked(message.filePath);
        break;
    case MessageType::VOICE:
        emit voiceClicked(message.filePath, message.messageId);
        break;
    case MessageType::TEXT:
        emit textClicked(message.content);
        break;
    default:
        break;
    }
    return true;
}

void ChatMessageDelegate::paintTextMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                           const Message &message, bool isOwn) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 基本参数设置
    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    const int bubblePadding = BUBBLE_PADDING;
    const int maxBubbleWidth = option.rect.width() * 0.6;
    const int timeSpacing = TIME_SPACING;
    bool isOwnMessage = isOwn; // 需要根据实际情况调整

    // 计算头像位置绘制头像
    QRect avatarRect;
    if (isOwnMessage) {
        avatarRect = QRect(option.rect.right() - margin - avatarSize,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    } else {
        avatarRect = QRect(option.rect.left() + margin,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    }
    paintAvatar(painter, avatarRect, message);

    // 计算文本尺寸
    QFont contentFont = option.font;
    contentFont.setPointSizeF(10.5);
    contentFont.setFamily(QStringLiteral("微软雅黑"));
    QFontMetrics contentMetrics(contentFont);
    QRect textRect = contentMetrics.boundingRect(QRect(0, 0, maxBubbleWidth - 2 * bubblePadding, 10000),
                                                 Qt::TextWordWrap, message.content);
    // 计算气泡尺寸
    int bubbleWidth = qMin(textRect.width() + 2 * bubblePadding, maxBubbleWidth);
    int bubbleHeight = textRect.height() + 2 * bubblePadding;

    // 计算气泡位置
    QRect bubbleRect;
    QRect contentRect;

    if (isOwnMessage) {
        bubbleRect = QRect(avatarRect.left() - margin - bubbleWidth, avatarRect.top(),
                           bubbleWidth, bubbleHeight);
        contentRect = bubbleRect.adjusted(bubblePadding, bubblePadding, -bubblePadding, -bubblePadding);
    } else {
        bubbleRect = QRect(avatarRect.right() + margin, avatarRect.top(),
                           bubbleWidth, bubbleHeight);
        contentRect = bubbleRect.adjusted(bubblePadding, bubblePadding, -bubblePadding, -bubblePadding);
    }

    // 绘制气泡
    painter->setPen(Qt::NoPen);
    if (isOwnMessage) {
        painter->setBrush(QColor(149, 236, 105));
    } else {
        painter->setBrush(Qt::white);
    }
    painter->drawRoundedRect(bubbleRect, 5, 5);

    // 绘制气泡小三角
    QPolygon triangle;
    if (isOwnMessage) {
        triangle << QPoint(bubbleRect.right(), bubbleRect.top() + 20)
        << QPoint(bubbleRect.right() + 6, bubbleRect.top() + 15)
        << QPoint(bubbleRect.right(), bubbleRect.top() + 10);
    } else {
        triangle << QPoint(bubbleRect.left(), bubbleRect.top() + 20)
        << QPoint(bubbleRect.left() - 6, bubbleRect.top() + 15)
        << QPoint(bubbleRect.left(), bubbleRect.top() + 10);
    }
    painter->drawPolygon(triangle);

    // 绘制消息内容
    painter->setPen(Qt::black);
    painter->setFont(contentFont);
    painter->drawText(contentRect, Qt::AlignLeft | Qt::TextWordWrap, message.content);

    // 绘制时间戳
    QRect timeRect(bubbleRect.left(), bubbleRect.bottom() + timeSpacing,
                   bubbleRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintImageMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                            const Message &message, bool isOwn) const
{
    QPixmap  thumbnail = thumbnailManager->getThumbnail(message.filePath, QSize(200, 300),
                                        MediaType::ImageThumb,0, message.thumbnailPath);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    bool isOwnMessage = isOwn;

    QRect avatarRect;
    if (isOwnMessage) {
        avatarRect = QRect(option.rect.right() - margin - avatarSize,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    } else {
        avatarRect = QRect(option.rect.left() + margin,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    }
    paintAvatar(painter, avatarRect, message);

    // 绘制缩略图
    QRect imageRect;
    if (isOwnMessage) {
        imageRect = QRect(avatarRect.left() - thumbnail.width() - margin,
                          avatarRect.top(),
                          thumbnail.width(), thumbnail.height());
    } else {
        imageRect = QRect(avatarRect.right() + margin,
                          avatarRect.top(),
                          thumbnail.width(), thumbnail.height());
    }
    painter->drawPixmap(imageRect, thumbnail);

    // 时间
    QRect timeRect(imageRect.left(), imageRect.bottom() + margin,
                   imageRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintVideoMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                            const Message &message,bool isOwn) const
{
    QPixmap thumbnail = thumbnailManager->getThumbnail(message.filePath, QSize(200, 300),
                                          MediaType::VideoThumb, 0 , message.thumbnailPath);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int margin = MARGIN;
    const int avatarSize = AVATAR_SIZE;
    bool isOwnMessage = isOwn;

    QRect avatarRect;
    if (isOwnMessage) {
        avatarRect = QRect(option.rect.right() - margin - avatarSize,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    } else {
        avatarRect = QRect(option.rect.left() + margin,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    }
    paintAvatar(painter, avatarRect, message);

    QRect videoRect;

    if (isOwnMessage) {
        videoRect = QRect(avatarRect.left() - margin - thumbnail.width(),
                            avatarRect.top(), thumbnail.width(), thumbnail.height());
    } else {
        videoRect = QRect(avatarRect.right() + margin, avatarRect.top(),
                            thumbnail.width(), thumbnail.height());
    }
    painter->drawPixmap(videoRect, thumbnail);

    // 绘制video时长
    if (message.duration > 0) {
        painter->setPen(Qt::white);
        painter->setBrush(QColor(200, 200, 200, 150));
        QFont durationFont = option.font;
        durationFont.setPointSize(8);
        painter->setFont(durationFont);
        QString durationText = message.formattedDuration();
        QRect durationRect(videoRect.left() + 10, videoRect.bottom() - 20,
                           videoRect.width() - 10, 15);
        painter->drawText(durationRect, Qt::AlignLeft | Qt::AlignVCenter, durationText);
    }

    // 时间
    QRect timeRect(videoRect.left(), videoRect.bottom() + margin, videoRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintFileMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                           const Message &message, bool isOwn) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 基本参数设置
    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    const int bubblePadding = 12;
    const int fileBubbleWidth = FILE_BUBBLE_WIDTH;
    const int fileBubbleHeight = FILE_BUBBLE_HEIGHT;
    const int iconWidth = ICON_WIDTH;
    const int iconHeight = ICON_HEIGHT;

    bool isOwnMessage = isOwn;

    // 计算头像位置
    QRect avatarRect;
    if (isOwnMessage) {
        avatarRect = QRect(option.rect.right() - margin - avatarSize,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    } else {
        avatarRect = QRect(option.rect.left() + margin,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    }
    paintAvatar(painter, avatarRect, message);

    // 计算文件气泡位置
    QRect fileBubbleRect;
    if (isOwnMessage) {
        fileBubbleRect = QRect(avatarRect.left() - margin - fileBubbleWidth,
                               avatarRect.top(),
                               fileBubbleWidth, fileBubbleHeight);
    } else {
        fileBubbleRect = QRect(avatarRect.right() + margin,
                               avatarRect.top(),
                               fileBubbleWidth, fileBubbleHeight);
    }

    // 检查文件是否存在
    bool fileExists = QFileInfo::exists(message.filePath);

    // 绘制文件气泡背景
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::white);

    painter->drawRoundedRect(fileBubbleRect, 5, 5);

    // 绘制气泡小三角
    QPolygon triangle;
    if (isOwnMessage) {
        triangle << QPoint(fileBubbleRect.right(), fileBubbleRect.top() + 20)
        << QPoint(fileBubbleRect.right() + 6, fileBubbleRect.top() + 15)
        << QPoint(fileBubbleRect.right(), fileBubbleRect.top() + 10);
    } else {
        triangle << QPoint(fileBubbleRect.left(), fileBubbleRect.top() + 20)
        << QPoint(fileBubbleRect.left() - 6, fileBubbleRect.top() + 15)
        << QPoint(fileBubbleRect.left(), fileBubbleRect.top() + 10);
    }
    painter->drawPolygon(triangle);

    // 文件图标区域
    QRect iconRect(fileBubbleRect.right() - bubblePadding - iconWidth,
                   fileBubbleRect.top() + bubblePadding,
                   iconWidth, iconHeight);

    // 绘制文件类型图标
    QFileInfo fileInfo(message.filePath);
    QString fileName = fileInfo.fileName();
    if (fileName.isEmpty()) {
        fileName = message.content;
    }
    QString fileExtension = fileInfo.suffix().toLower();
    paintFileIcon(painter, iconRect, fileExtension);

    // 文本区域
    QRect textRect(fileBubbleRect.left() + bubblePadding,
                   fileBubbleRect.top() + bubblePadding,
                   fileBubbleWidth - iconWidth - 3 * bubblePadding,
                   fileBubbleHeight - 2 * bubblePadding);

    // 绘制文件名
    painter->setPen(Qt::black);
    QFont fileNameFont = option.font;
    fileNameFont.setPointSizeF(10.2);
    fileNameFont.setFamily("微软雅黑");
    painter->setFont(fileNameFont);

    // 文件名省略处理
    QString displayName = fileName;
    QFontMetrics nameMetrics(fileNameFont);
    if (nameMetrics.horizontalAdvance(fileName) > textRect.width()) {
        displayName = nameMetrics.elidedText(fileName, Qt::ElideRight, textRect.width());
    }
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, displayName);

    // 绘制文件大小
    QString sizeStr = message.formattedFileSize();

    QFont sizeFont = option.font;
    sizeFont.setPointSizeF(9);
    painter->setFont(sizeFont);
    painter->setPen(QColor(150, 150, 150));

    QRect sizeRect = textRect.adjusted(0, nameMetrics.height() + bubblePadding/2, 0, 0);
    painter->drawText(sizeRect, Qt::AlignLeft | Qt::AlignTop, sizeStr);

    // 绘制底部横线和来源标识
    QFont sourceFont = option.font;
    sourceFont.setPointSizeF(8.5);
    sourceFont.setFamily("微软雅黑");
    painter->setFont(sourceFont);
    painter->setPen(QColor(200, 200, 200));

    // 横线
    QLine dividerLine(fileBubbleRect.left() + bubblePadding, fileBubbleRect.bottom() - 25,
                      fileBubbleRect.right() - bubblePadding, fileBubbleRect.bottom() - 25);
    painter->drawLine(dividerLine);

    // 标识文字
    painter->setPen(QColor(150, 150, 150));
    painter->drawText(QRect(textRect.left(), fileBubbleRect.bottom() - 20,
                            textRect.width(), 15),
                      Qt::AlignLeft | Qt::AlignVCenter, "微信电脑版");

    // 如果文件不存在，绘制暗色遮罩和提示文字
    if (!fileExists) {
        painter->save();

        // 绘制暗色遮罩（半透明黑色）
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 128)); // 半透明黑色
        painter->drawRoundedRect(fileBubbleRect, 5, 5);

        // 绘制提示文字
        QFont warningFont = option.font;
        warningFont.setPointSizeF(9);
        warningFont.setFamily("微软雅黑");
        warningFont.setBold(true);
        painter->setFont(warningFont);
        painter->setPen(Qt::white);

        QString warningText = "文件已过期或删除";
        QFontMetrics warningMetrics(warningFont);
        int warningWidth = warningMetrics.horizontalAdvance(warningText);
        QRect warningRect(fileBubbleRect.center().x() - warningWidth / 2,
                          fileBubbleRect.center().y() - warningMetrics.height() / 2,
                          warningWidth, warningMetrics.height());

        painter->drawText(warningRect, Qt::AlignCenter, warningText);

        // 恢复 painter 状态
        painter->restore();
    }

    // 绘制时间戳
    QRect timeRect = QRect(fileBubbleRect.left(), fileBubbleRect.bottom() + margin,
                           fileBubbleRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintVoiceMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                            const Message &message, bool isOwn) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 基本参数设置
    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    const int bubblePadding = 12;
    const int minVoiceBubbleWidth = MIN_VOICE_BUBBLE_WIDTH;
    const int maxVoiceBubbleWidth = MAX_VOICE_BUBBLE_WIDTH;
    const int voiceBubbleHeight = VOICE_BUBBLE_HEIGHT;
    const int playButtonSize = PLAY_BUTTON_SIZE;
    const int waveformHeight = WAVEFORM_HEIGHT;

    bool isOwnMessage = isOwn;
    int duration = message.duration; // 秒数

    // 计算头像位置
    QRect avatarRect;
    if (isOwnMessage) {
        avatarRect = QRect(option.rect.right() - margin - avatarSize,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    } else {
        avatarRect = QRect(option.rect.left() + margin,
                           option.rect.top() + margin,
                           avatarSize, avatarSize);
    }
    paintAvatar(painter, avatarRect, message);

    // 根据时长计算气泡宽度
    int voiceBubbleWidth = qMin(maxVoiceBubbleWidth,
                                minVoiceBubbleWidth + duration * 3);

    // 计算语音气泡位置
    QRect voiceBubbleRect;
    if (isOwnMessage) {
        voiceBubbleRect = QRect(avatarRect.left() - margin - voiceBubbleWidth,
                                avatarRect.top() + (avatarSize - voiceBubbleHeight) / 2,
                                voiceBubbleWidth, voiceBubbleHeight);
    } else {
        voiceBubbleRect = QRect(avatarRect.right() + margin,
                                avatarRect.top() + (avatarSize - voiceBubbleHeight) / 2,
                                voiceBubbleWidth, voiceBubbleHeight);
    }

    // 绘制语音气泡背景（微信样式：自己绿色，对方白色）
    painter->setPen(Qt::NoPen);
    if (isOwnMessage) {
        painter->setBrush(QColor(0x07, 0xC1, 0x60)); // 微信绿色
    } else {
        painter->setBrush(Qt::white);
    }
    painter->drawRoundedRect(voiceBubbleRect, 5, 5);

    // 绘制气泡小三角
    QPolygon triangle;
    if (isOwnMessage) {
        triangle << QPoint(voiceBubbleRect.right(), voiceBubbleRect.top() + 22)
        << QPoint(voiceBubbleRect.right() + 6, voiceBubbleRect.top() + 17)
        << QPoint(voiceBubbleRect.right(), voiceBubbleRect.top() + 12);
        painter->setBrush(QColor(0x07, 0xC1, 0x60));
    } else {
        triangle << QPoint(voiceBubbleRect.left(), voiceBubbleRect.top() + 22)
        << QPoint(voiceBubbleRect.left() - 6, voiceBubbleRect.top() + 17)
        << QPoint(voiceBubbleRect.left(), voiceBubbleRect.top() + 12);
        painter->setBrush(Qt::white);
    }
    painter->drawPolygon(triangle);

    AudioPlayer* player = AudioPlayer::instance();
    bool isPlaying = player->isPlaying() && (player->currentMessageId() == message.messageId);
    // 绘制播放按钮和波形
    paintPlayButtonAndWaveform(painter, voiceBubbleRect, isOwnMessage, isPlaying, message.messageId);
    paintDurationText(painter, voiceBubbleRect, duration, isOwnMessage);
    // 绘制时间戳
    QRect timeRect = QRect(voiceBubbleRect.left(), voiceBubbleRect.bottom() + margin,
                           voiceBubbleRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintAvatar(QPainter *painter, const QRect &avatarRect,
                                      const Message &message) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPixmap avatarPixmap = QPixmap();
    if(QFileInfo::exists(message.avatar)){
        avatarPixmap = thumbnailManager->getThumbnail(message.avatar,
                                                 avatarRect.size(), 
                                                 MediaType::Avatar, 5);
    }

    if(!avatarPixmap.isNull()) {
        painter->drawPixmap(avatarRect, avatarPixmap);
    } else {
        // 绘制默认头像
        QPainterPath path;
        path.addRoundedRect(QRectF(avatarRect), 5, 5);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(210, 210, 210));
        painter->drawPath(path);

        QFont iniFont = painter->font();
        iniFont.setBold(true);
        iniFont.setPointSize(15);
        painter->setFont(iniFont);
        painter->setPen(QColor(100, 100, 100));
        QString initial = message.senderName.isEmpty() ? "U" : message.senderName.left(1);
        painter->drawText(avatarRect, Qt::AlignCenter, initial.toUpper());
    }

    painter->restore();
}

void ChatMessageDelegate::paintTime(QPainter *painter, const QRect &rect, const QStyleOptionViewItem &option,
                                    const Message &message, bool isOwnMessage) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 绘制时间戳
    QString timeText = FormatTime(message.timestamp);
    QFont timeFont = option.font;
    timeFont.setPointSizeF(7.5);
    painter->setFont(timeFont);
    QFontMetrics timeMetrics(timeFont);
    int timeHeight = timeMetrics.height();
    painter->setPen(QColor(150, 150, 150));
    QRect timeRect(rect.left(), rect.top(), rect.width(), timeHeight);

    if (isOwnMessage) {
        painter->drawText(timeRect, Qt::AlignRight | Qt::AlignTop, timeText);
    } else {
        painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignTop, timeText);
    }

    painter->restore();
}

void ChatMessageDelegate::paintFileIcon(QPainter *painter, const QRect &fileRect,
                                        const QString &extension) const
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
    filePath.moveTo(fileRect.topLeft());    // A点：左上角
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

void ChatMessageDelegate::paintPlayButtonAndWaveform(QPainter *painter,
                                                     const QRect &bubbleRect,
                                                     bool isOwnMessage,
                                                     bool isPlaying,
                                                     const qint64& messageId) const
{
    const int playButtonSize = PLAY_BUTTON_SIZE;

    // 计算播放按钮位置（同原有代码）
    QRect playButtonRect;
    if (isOwnMessage) {
        playButtonRect = QRect(bubbleRect.right() - playButtonSize - 8,
                               bubbleRect.center().y() - playButtonSize/2,
                               playButtonSize, playButtonSize);
    } else {
        playButtonRect = QRect(bubbleRect.left() + 8,
                               bubbleRect.center().y() - playButtonSize/2,
                               playButtonSize, playButtonSize);
    }

    // 绘制圆形按钮背景（颜色与原有逻辑一致）
    painter->setPen(Qt::NoPen);
    if (isOwnMessage) {
        painter->setBrush(Qt::white);
    } else {
        painter->setBrush(QColor(0x07, 0xC1, 0x60));
    }
    painter->drawEllipse(playButtonRect);

    // 根据播放状态绘制图标
    painter->setBrush(isOwnMessage ? QColor(0x07, 0xC1, 0x60) : Qt::white);
    if (isPlaying) {
        // 绘制暂停图标（两个竖条）
        int barWidth = 3;
        int barHeight = 10;
        int centerX = playButtonRect.center().x();
        int centerY = playButtonRect.center().y();
        QRect leftBar(centerX - barWidth - 2, centerY - barHeight/2, barWidth, barHeight);
        QRect rightBar(centerX + 2, centerY - barHeight/2, barWidth, barHeight);
        painter->drawRect(leftBar);
        painter->drawRect(rightBar);
    } else {
        // 绘制播放三角形（同原有逻辑）
        QPolygon playIcon;
        int iconSize = 8;
        if (isOwnMessage) {
            playIcon << QPoint(playButtonRect.center().x() - 2, playButtonRect.center().y() - iconSize/2)
            << QPoint(playButtonRect.center().x() - 2, playButtonRect.center().y() + iconSize/2)
            << QPoint(playButtonRect.center().x() + iconSize/2, playButtonRect.center().y());
        } else {
            playIcon << QPoint(playButtonRect.center().x() + 2, playButtonRect.center().y() - iconSize/2)
            << QPoint(playButtonRect.center().x() + 2, playButtonRect.center().y() + iconSize/2)
            << QPoint(playButtonRect.center().x() - iconSize/2, playButtonRect.center().y());
        }
        painter->drawPolygon(playIcon);
    }

    // 绘制波形（传入播放状态和消息Id）
    QRect waveformRect;
    if (isOwnMessage) {
        waveformRect = QRect(playButtonRect.left() - 60 - 5,
                             bubbleRect.center().y() - WAVEFORM_HEIGHT/2,
                             60, WAVEFORM_HEIGHT);
    } else {
        waveformRect = QRect(playButtonRect.right() + 5,
                             bubbleRect.center().y() - WAVEFORM_HEIGHT/2,
                             60, WAVEFORM_HEIGHT);
    }
    paintVoiceWaveform(painter, waveformRect, isOwnMessage, isPlaying, messageId);
}

void ChatMessageDelegate::paintVoiceWaveform(QPainter *painter,
                                             const QRect &rect,
                                             bool isOwnMessage,
                                             bool isPlaying,
                                             const qint64& messageId) const
{
    QList<int> heights;
    if (isPlaying) {
        // 从播放器获取当前波形数据
        heights = AudioPlayer::instance()->currentWaveform();
    } else {
        // 默认静态波形（与原逻辑一致）
        heights = {8, 12, 16, 12};
    }

    // 绘制条形图（保持原有绘制逻辑，但使用 heights 数据）
    const int barCount = heights.size();
    const int barWidth = 3;
    const int gap = 2;
    int totalWidth = barCount * barWidth + (barCount - 1) * gap;
    int startX = rect.left() + (rect.width() - totalWidth) / 2;
    int centerY = rect.center().y();

    painter->setPen(Qt::NoPen);
    for (int i = 0; i < barCount; ++i) {
        int barHeight = heights[i];
        QRect barRect(startX + i * (barWidth + gap),
                      centerY - barHeight/2,
                      barWidth, barHeight);
        if (isOwnMessage) {
            painter->setBrush(Qt::white);
        } else {
            painter->setBrush(QColor(0x07, 0xC1, 0x60));
        }
        painter->drawRect(barRect);
    }
}

void ChatMessageDelegate::paintDurationText(QPainter *painter, const QRect &bubbleRect,
                                            int duration, bool isOwnMessage) const
{
    const int playButtonSize = PLAY_BUTTON_SIZE;
    QString durationStr = QString("%1\"") .arg(duration);
    painter->setFont(QFont("微软雅黑", 9));

    if (isOwnMessage) {
        painter->setPen(Qt::white);
        painter->drawText(bubbleRect.adjusted(8, 0, -playButtonSize-20, 0),
                          Qt::AlignLeft | Qt::AlignVCenter, durationStr);
    } else {
        painter->setPen(QColor(100, 100, 100));
        painter->drawText(bubbleRect.adjusted(playButtonSize+20, 0, -8, 0),
                          Qt::AlignRight | Qt::AlignVCenter, durationStr);
    }
}

QSize ChatMessageDelegate::calculateTextSize(const QString &text, const QFont &font,
                                             int maxWidth) const
{
    if (text.isEmpty()) return QSize(0, 0);

    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(
        QRect(0, 0, maxWidth, 0),
        Qt::TextWordWrap | Qt::AlignLeft,
        text
        );
    return textRect.size();
}


QRect ChatMessageDelegate::getClickableRect(const QStyleOptionViewItem &option,
                                            const Message &message, const bool &isOwn) const
{
    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    bool isOwnMessage = isOwn;

    switch (message.type) {
    case MessageType::IMAGE:
    case MessageType::VIDEO: {
        QPixmap thumbnail = thumbnailManager->getThumbnail(message.filePath, QSize(200, 300),
                                                   MediaType::ImageThumb,0, message.thumbnailPath);
        if (isOwnMessage) {
            return QRect(option.rect.right() - avatarSize - thumbnail.width() - 2 * margin,
                         option.rect.top() + margin, thumbnail.width(), thumbnail.height());
        } else {
            return QRect(option.rect.left() + avatarSize + 2 * margin,
                         option.rect.top() + margin, thumbnail.width(), thumbnail.height());
        }
    }
    case MessageType::FILE: {
        const int fileBubbleWidth = FILE_BUBBLE_WIDTH;
        const int fileBubbleHeight = FILE_BUBBLE_HEIGHT;
        QRect fileBubbleRect;
        if (isOwnMessage) {
            fileBubbleRect = QRect(option.rect.right() - 2 * margin - avatarSize - fileBubbleWidth,
                                   option.rect.top() + margin, fileBubbleWidth, fileBubbleHeight);
        } else {
            fileBubbleRect = QRect(option.rect.left() + 2 * margin + avatarSize,
                                   option.rect.top() + margin, fileBubbleWidth, fileBubbleHeight);
        }
        return fileBubbleRect;
    }
    case MessageType::VOICE: {
        const int minVoiceBubbleWidth = MIN_VOICE_BUBBLE_WIDTH;
        const int maxVoiceBubbleWidth = MAX_VOICE_BUBBLE_WIDTH;
        const int voiceBubbleHeight = VOICE_BUBBLE_HEIGHT;
        int duration = message.duration; // 秒数
        int voiceBubbleWidth = qMin(maxVoiceBubbleWidth,
                                    minVoiceBubbleWidth + duration * 3);
        QRect voiceBubbleRect;
        if (isOwnMessage) {
            voiceBubbleRect = QRect(option.rect.right() - avatarSize - 2 * margin - voiceBubbleWidth,
                                    option.rect.top() + margin + (avatarSize - voiceBubbleHeight) / 2,
                                    voiceBubbleWidth, voiceBubbleHeight);
        } else {
            voiceBubbleRect = QRect(option.rect.left() + avatarSize + 2 * margin,
                                    option.rect.top() + margin + (avatarSize - voiceBubbleHeight) / 2,
                                    voiceBubbleWidth, voiceBubbleHeight);
        }
        return voiceBubbleRect;
    }
    case MessageType::TEXT: {
        // 计算文本气泡的矩形区域
        const int maxBubbleWidth = option.rect.width() * 0.6;
        const int bubblePadding = BUBBLE_PADDING;

        // 计算文本尺寸
        QFont contentFont;
        contentFont.setPointSizeF(10.5);
        contentFont.setFamily(QStringLiteral("微软雅黑"));
        QSize textSize = calculateTextSize(message.content, contentFont, maxBubbleWidth - 2 * bubblePadding);

        // 计算气泡尺寸
        int bubbleWidth = qMin(textSize.width() + 2 * bubblePadding, maxBubbleWidth);
        int bubbleHeight = textSize.height() + 2 * bubblePadding;

        // 计算气泡位置
        QRect bubbleRect;
        if (isOwnMessage) {
            bubbleRect = QRect(option.rect.right() - margin - avatarSize - margin - bubbleWidth,
                               option.rect.top() + margin,
                               bubbleWidth, bubbleHeight);
        } else {
            bubbleRect = QRect(option.rect.left() + margin + avatarSize + margin,
                               option.rect.top() + margin,
                               bubbleWidth, bubbleHeight);
        }
        return bubbleRect;
    }
    default:
        return QRect();
    }
}


void ChatMessageDelegate::onMediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type)
{
    if(ChatMessageListView* view = qobject_cast<ChatMessageListView*>(parent())) {

        view->viewport()->update();
        QAbstractItemModel* model = view->model();
        if (model) {
            QModelIndex topLeft = model->index(0, 0);
            QModelIndex bottomRight = model->index(model->rowCount() - 1, 0);
            emit model->dataChanged(topLeft, bottomRight);
        }
    }
}

