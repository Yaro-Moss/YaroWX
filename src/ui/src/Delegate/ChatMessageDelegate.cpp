#include "ChatMessageDelegate.h"
#include "ContactController.h"
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
    , m_contactController(nullptr)
{
    thumbnailManager = ThumbnailResourceManager::instance();
    connect(thumbnailManager, &ThumbnailResourceManager::mediaLoaded,
            this, &ChatMessageDelegate::onMediaLoaded);
}

ChatMessageDelegate::~ChatMessageDelegate()
{
    // 停止所有动画定时器
    for (int timerId : m_animationTimers.values()) {
        killTimer(timerId);
    }
    m_animationTimers.clear();
    m_animationAngles.clear();
}

void ChatMessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制选中背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, QColor(220, 220, 220));
    }

    Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
    bool isOwn = index.data(ChatMessagesModel::IsOwnRole).toBool();

    switch (static_cast<MessageType>(message.typeValue())) {
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
        paintFileMessage(painter, option, message, isOwn);
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

    QFont timeFont = option.font;
    timeFont.setPointSizeF(7.5);
    QFontMetrics timeMetrics(timeFont);
    int timeHeight = timeMetrics.height();

    switch (static_cast<MessageType>(message.typeValue())) {
    case MessageType::TEXT: {
        int maxBubbleWidth = width * 0.6;
        QFont font = option.font;
        font.setPointSizeF(10.5);
        font.setFamily(QStringLiteral("微软雅黑"));
        QSize textSize = calculateTextSize(message.contentValue(), font, maxBubbleWidth - 2 * bubblePadding);
        int bubbleHeight = textSize.height() + 2 * bubblePadding;
        int avatarAreaHeight = avatarSize + 2 * margin;
        int contentAreaHeight = bubbleHeight + timeHeight + 2 * margin;
        return QSize(width, qMax(avatarAreaHeight, contentAreaHeight));
    }
    case MessageType::IMAGE:
    case MessageType::VIDEO: {
        const int margin = MARGIN;
        QFont timeFont = option.font;
        timeFont.setPointSizeF(7.5);
        QFontMetrics timeMetrics(timeFont);
        int timeHeight = timeMetrics.height();

        auto thumbStatus = message.thumbDownloadStatus();
        int thumbHeight = 150;
        if (thumbStatus == DownloadStatus::COMPLETED) {
            QPixmap thumb = thumbnailManager->getThumbnail(message.file_pathValue(), QSize(200, 300),
                                                           MediaType::ImageThumb, 0, message.thumbnail_pathValue());
            thumbHeight = thumb.height();
            if (thumbHeight <= 0) thumbHeight = 150;
        }
        return QSize(width, thumbHeight + timeHeight + 2 * margin);
    }
    case MessageType::FILE: {
        int totalHeight = FILE_BUBBLE_HEIGHT + timeHeight + 2 * margin;
        int avatarAreaHeight = avatarSize + 2 * margin;
        return QSize(width, qMax(totalHeight, avatarAreaHeight));
    }
    case MessageType::VOICE: {
        int totalHeight = VOICE_BUBBLE_HEIGHT + timeHeight + 2 * margin;
        int avatarAreaHeight = avatarSize + 2 * margin;
        return QSize(width, qMax(totalHeight, avatarAreaHeight));
    }
    default:
        return QSize(100, 30 + 2 * margin);
    }
}

QRect ChatMessageDelegate::getAvatarRect(const QStyleOptionViewItem &option, bool isOwn) const
{
    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    if (isOwn) {
        return QRect(option.rect.right() - margin - avatarSize,
                     option.rect.top() + margin,
                     avatarSize, avatarSize);
    } else {
        return QRect(option.rect.left() + margin,
                     option.rect.top() + margin,
                     avatarSize, avatarSize);
    }
}

bool ChatMessageDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                      const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent->button() == Qt::RightButton) {
            Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
            bool isOwn = index.data(ChatMessagesModel::IsOwnRole).toBool();
            QRect clickableRect = getClickableRect(option, message, isOwn);
            if (clickableRect.contains(mouseEvent->pos())) {
                QPoint globalPos = option.widget->mapToGlobal(mouseEvent->pos());
                emit rightClicked(globalPos, message);
                return true;
            }
        } else if (mouseEvent->button() == Qt::LeftButton) {
            bool isOwn = index.data(ChatMessagesModel::IsOwnRole).toBool();
            QRect avatarRect = getAvatarRect(option, isOwn);
            if (avatarRect.contains(mouseEvent->pos())) {
                Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
                emit avatarClicked(message.sender_idValue());
                return true;
            }
            return handleLeftClick(mouseEvent, option, index);
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

bool ChatMessageDelegate::handleLeftClick(QMouseEvent *mouseEvent, const QStyleOptionViewItem &option,
                                          const QModelIndex &index)
{
    Message message = index.data(ChatMessagesModel::FullMessageRole).value<Message>();
    bool isOwn = index.data(ChatMessagesModel::IsOwnRole).toBool();

    QRect clickableRect = getClickableRect(option, message, isOwn);
    if (!clickableRect.contains(mouseEvent->pos())) {
        return false;
    }

    MessageType type = static_cast<MessageType>(message.typeValue());

    // 图片和视频的特殊处理：检查下载状态
    if (type == MessageType::IMAGE || type == MessageType::VIDEO) {
        auto thumbStatus = message.thumbDownloadStatus();
        auto fileStatus = message.fileDownloadStatus();

        // 缩略图未就绪：不允许点击（缩略图自动下载，此处可忽略）
        if (thumbStatus != DownloadStatus::COMPLETED) {
            return false;
        }

        // 原文件未下载：请求下载
        if (fileStatus == DownloadStatus::NOT_DOWNLOADED) {
            emit downloadRequested(message.message_idValue());
            return false;
        }
        // 原文件下载中或失败：不允许打开
        if (fileStatus != DownloadStatus::COMPLETED) {
            return false;
        }

        // 两者都已下载：触发打开信号
        emit mediaClicked(message.message_idValue(), message.conversation_idValue());
        return true;
    }

    else {
        // 其他消息类型保持原有逻辑
        switch (type) {
        case MessageType::FILE: {
            auto fileStatus = message.fileDownloadStatus();
            bool fileExists = QFileInfo::exists(message.file_pathValue());

            if (!fileExists && fileStatus == DownloadStatus::NOT_DOWNLOADED) {
                emit downloadRequested(message.message_idValue());
                return false;   // 已请求下载，本次点击不打开
            }
            else if (fileStatus == DownloadStatus::DOWNLOADING || fileStatus == DownloadStatus::FAILED) {
                return false;   // 下载中或失败时不打开
            }
            else {
                emit fileClicked(message.file_pathValue());
                return true;
            }
        }
        case MessageType::VOICE:{
            auto fileStatus = message.fileDownloadStatus();
            bool fileExists = QFileInfo::exists(message.file_pathValue());

            if (!fileExists && fileStatus == DownloadStatus::NOT_DOWNLOADED) {
                emit downloadRequested(message.message_idValue());
                return false;   // 已请求下载，本次点击不打开
            }
            else if (fileStatus == DownloadStatus::DOWNLOADING || fileStatus == DownloadStatus::FAILED) {
                return false;   // 下载中或失败时不打开
            }
            emit voiceClicked(message.file_pathValue(), message.message_idValue());
            break;
        }
        case MessageType::TEXT:
            emit textClicked(message.contentValue());
            break;
        default:
            break;
        }
        return true;
    }
}

void ChatMessageDelegate::paintTextMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                           const Message &message, bool isOwn) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    const int bubblePadding = BUBBLE_PADDING;
    const int maxBubbleWidth = option.rect.width() * 0.6;
    const int timeSpacing = TIME_SPACING;
    bool isOwnMessage = isOwn;

    QRect avatarRect = getAvatarRect(option, isOwnMessage);
    paintAvatar(painter, avatarRect, message);

    QFont contentFont = option.font;
    contentFont.setPointSizeF(10.5);
    contentFont.setFamily(QStringLiteral("微软雅黑"));
    QFontMetrics contentMetrics(contentFont);
    QRect textRect = contentMetrics.boundingRect(QRect(0, 0, maxBubbleWidth - 2 * bubblePadding, 10000),
                                                 Qt::TextWordWrap, message.contentValue());

    int bubbleWidth = qMin(textRect.width() + 2 * bubblePadding, maxBubbleWidth);
    int bubbleHeight = textRect.height() + 2 * bubblePadding;

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

    painter->setPen(Qt::NoPen);
    painter->setBrush(isOwnMessage ? QColor(149, 236, 105) : Qt::white);
    painter->drawRoundedRect(bubbleRect, 5, 5);

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

    painter->setPen(Qt::black);
    painter->setFont(contentFont);
    painter->drawText(contentRect, Qt::AlignLeft | Qt::TextWordWrap, message.contentValue());

    QRect timeRect(bubbleRect.left(), bubbleRect.bottom() + timeSpacing,
                   bubbleRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintImageMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                            const Message &message, bool isOwn) const
{
    auto thumbStatus = message.thumbDownloadStatus();
    auto fileStatus = message.fileDownloadStatus();

    // 决定实际显示的图片：缩略图未准备好时显示灰色占位图
    QSize thumbSize(150, 150);
    QPixmap displayPixmap;
    if (thumbStatus != DownloadStatus::COMPLETED) {
        displayPixmap = QPixmap(thumbSize);
        displayPixmap.fill(QColor(200, 200, 200));
    }else if(fileStatus!=DownloadStatus::COMPLETED){
        QImage image(message.thumbnail_pathValue());
        displayPixmap = QPixmap::fromImage(image.scaled(QSize(200,300), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else {
        displayPixmap = thumbnailManager->getThumbnail(message.file_pathValue(), QSize(200, 300),
                                                       MediaType::ImageThumb, 0, message.thumbnail_pathValue());
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int margin = MARGIN;
    const int avatarSize = AVATAR_SIZE;
    bool isOwnMessage = isOwn;

    QRect avatarRect = getAvatarRect(option, isOwnMessage);
    paintAvatar(painter, avatarRect, message);

    QRect imageRect;
    if (isOwnMessage) {
        imageRect = QRect(avatarRect.left() - displayPixmap.width() - margin,
                          avatarRect.top(),
                          displayPixmap.width(), displayPixmap.height());
    } else {
        imageRect = QRect(avatarRect.right() + margin,
                          avatarRect.top(),
                          displayPixmap.width(), displayPixmap.height());
    }

    painter->drawPixmap(imageRect, displayPixmap);

    // 绘制覆盖层（仅当缩略图已就绪时才可能显示下载/失败/加载等状态）
    if (thumbStatus == DownloadStatus::COMPLETED) {
        paintMediaOverlay(painter, imageRect, thumbStatus, fileStatus, message.message_idValue());
    } else if (thumbStatus == DownloadStatus::DOWNLOADING) {
        // 缩略图正在下载中，显示加载动画（覆盖灰色区域）
        paintMediaOverlay(painter, imageRect, thumbStatus, fileStatus, message.message_idValue());
    } else if (thumbStatus == DownloadStatus::FAILED) {
        // 缩略图下载失败，显示失败图标
        paintMediaOverlay(painter, imageRect, thumbStatus, fileStatus, message.message_idValue());
    }

    QRect timeRect(imageRect.left(), imageRect.bottom() + margin,
                   imageRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintVideoMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                            const Message &message, bool isOwn) const
{
    // 视频缩略图获取
    QPixmap thumbnail = thumbnailManager->getThumbnail(message.file_pathValue(), QSize(200, 300),
                                                       MediaType::VideoThumb, 0, message.thumbnail_pathValue());

    auto thumbStatus = message.thumbDownloadStatus();
    auto fileStatus = message.fileDownloadStatus();

    QSize thumbSize(200, 300);
    QPixmap displayPixmap;
    if (thumbStatus != DownloadStatus::COMPLETED) {
        displayPixmap = QPixmap(thumbSize);
        displayPixmap.fill(QColor(200, 200, 200));
    } else {
        displayPixmap = thumbnail;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int margin = MARGIN;
    const int avatarSize = AVATAR_SIZE;
    bool isOwnMessage = isOwn;

    QRect avatarRect = getAvatarRect(option, isOwnMessage);
    paintAvatar(painter, avatarRect, message);

    QRect videoRect;
    if (isOwnMessage) {
        videoRect = QRect(avatarRect.left() - displayPixmap.width() - margin,
                          avatarRect.top(),
                          displayPixmap.width(), displayPixmap.height());
    } else {
        videoRect = QRect(avatarRect.right() + margin,
                          avatarRect.top(),
                          displayPixmap.width(), displayPixmap.height());
    }

    painter->drawPixmap(videoRect, displayPixmap);

    // 绘制位置
    if (thumbStatus == DownloadStatus::COMPLETED) {
        paintMediaOverlay(painter, videoRect, thumbStatus, fileStatus, message.message_idValue());
    } else if (thumbStatus == DownloadStatus::DOWNLOADING) {
        paintMediaOverlay(painter, videoRect, thumbStatus, fileStatus, message.message_idValue());
    } else if (thumbStatus == DownloadStatus::FAILED) {
        paintMediaOverlay(painter, videoRect, thumbStatus, fileStatus, message.message_idValue());
    }

    // 视频时长（仅在视频本身已下载或已知时长时显示）
    if (message.durationValue() > 0) {
        painter->setPen(Qt::white);
        painter->setBrush(QColor(0, 0, 0, 150));
        QFont durationFont = option.font;
        durationFont.setPointSize(8);
        painter->setFont(durationFont);
        QString durationText = message.formattedDuration();
        QRect durationRect(videoRect.left() + 10, videoRect.bottom() - 20,
                           videoRect.width() - 20, 15);
        painter->drawText(durationRect, Qt::AlignLeft | Qt::AlignVCenter, durationText);
    }

    QRect timeRect(videoRect.left(), videoRect.bottom() + margin,
                   videoRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintFileMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                           const Message &message, bool isOwn) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    const int bubblePadding = 12;
    const int fileBubbleWidth = FILE_BUBBLE_WIDTH;
    const int fileBubbleHeight = FILE_BUBBLE_HEIGHT;
    const int iconWidth = ICON_WIDTH;
    const int iconHeight = ICON_HEIGHT;
    bool isOwnMessage = isOwn;

    // 获取文件下载状态及本地存在性
    auto fileStatus = message.fileDownloadStatus();
    bool fileExists = QFileInfo::exists(message.file_pathValue());

    // 有效状态：如果文件已存在，视为 COMPLETED；否则使用消息中记录的状态
    DownloadStatus effectiveStatus = fileExists ? DownloadStatus::COMPLETED : fileStatus;

    // 绘制头像
    QRect avatarRect = getAvatarRect(option, isOwnMessage);
    paintAvatar(painter, avatarRect, message);

    // 文件气泡矩形
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

    // 绘制气泡背景和三角形
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::white);
    painter->drawRoundedRect(fileBubbleRect, 5, 5);

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

    // 文件图标
    QRect iconRect(fileBubbleRect.right() - bubblePadding - iconWidth,
                   fileBubbleRect.top() + bubblePadding,
                   iconWidth, iconHeight);
    QFileInfo fileInfo(message.file_pathValue());
    QString fileName = fileInfo.fileName();
    if (fileName.isEmpty()) fileName = message.contentValue();
    QString fileExtension = fileInfo.suffix().toLower();
    paintFileIcon(painter, iconRect, fileExtension);

    // 文件名
    QRect textRect(fileBubbleRect.left() + bubblePadding,
                   fileBubbleRect.top() + bubblePadding,
                   fileBubbleWidth - iconWidth - 3 * bubblePadding,
                   fileBubbleHeight - 2 * bubblePadding);
    painter->setPen(Qt::black);
    QFont fileNameFont = option.font;
    fileNameFont.setPointSizeF(10.2);
    fileNameFont.setFamily("微软雅黑");
    painter->setFont(fileNameFont);

    QString displayName = fileName;
    QFontMetrics nameMetrics(fileNameFont);
    if (nameMetrics.horizontalAdvance(fileName) > textRect.width()) {
        displayName = nameMetrics.elidedText(fileName, Qt::ElideRight, textRect.width());
    }
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, displayName);

    // 文件大小
    QString sizeStr = message.formattedFileSize();
    QFont sizeFont = option.font;
    sizeFont.setPointSizeF(9);
    painter->setFont(sizeFont);
    painter->setPen(QColor(150, 150, 150));
    QRect sizeRect = textRect.adjusted(0, nameMetrics.height() + bubblePadding/2, 0, 0);
    painter->drawText(sizeRect, Qt::AlignLeft | Qt::AlignTop, sizeStr);

    // 底部分隔线和来源文字
    QFont sourceFont = option.font;
    sourceFont.setPointSizeF(8.5);
    sourceFont.setFamily("微软雅黑");
    painter->setFont(sourceFont);
    painter->setPen(QColor(200, 200, 200));
    QLine dividerLine(fileBubbleRect.left() + bubblePadding, fileBubbleRect.bottom() - 25,
                      fileBubbleRect.right() - bubblePadding, fileBubbleRect.bottom() - 25);
    painter->drawLine(dividerLine);
    painter->setPen(QColor(150, 150, 150));
    painter->drawText(QRect(textRect.left(), fileBubbleRect.bottom() - 20,
                            textRect.width(), 15),
                      Qt::AlignLeft | Qt::AlignVCenter, "微信电脑版");

    // ------------------------------------------------------------
    // 根据文件下载状态绘制覆盖层（下载箭头/加载动画/失败图标）
    // ------------------------------------------------------------
    if (effectiveStatus == DownloadStatus::NOT_DOWNLOADED ||
        effectiveStatus == DownloadStatus::DOWNLOADING ||
        effectiveStatus == DownloadStatus::FAILED) {

        int overlaySize = qMin(40, qMin(fileBubbleRect.width(), fileBubbleRect.height()) / 2);
        QRect overlayRect(fileBubbleRect.center().x() - overlaySize/2,
                          fileBubbleRect.center().y() - overlaySize/2,
                          overlaySize, overlaySize);

        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 180));  // 半透明黑色背景
        painter->drawEllipse(overlayRect);
        painter->setPen(Qt::white);
        QFont iconFont = painter->font();
        iconFont.setPointSize(overlaySize * 0.4);
        painter->setFont(iconFont);

        switch (effectiveStatus) {
        case DownloadStatus::NOT_DOWNLOADED:
            painter->drawText(overlayRect, Qt::AlignCenter, "↓");
            const_cast<ChatMessageDelegate*>(this)->stopAnimationForMessage(message.message_idValue());
            break;
        case DownloadStatus::DOWNLOADING:
            const_cast<ChatMessageDelegate*>(this)->startAnimationForMessage(message.message_idValue());
            {
                int angle = m_animationAngles.value(message.message_idValue(), 0);
                int arcSpan = 40 * 16;   // 40度圆弧
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(Qt::white, 3));
                painter->drawArc(overlayRect, angle * 16, arcSpan);
            }
            break;
        case DownloadStatus::FAILED:
            painter->drawText(overlayRect, Qt::AlignCenter, "✗");
            const_cast<ChatMessageDelegate*>(this)->stopAnimationForMessage(message.message_idValue());
            break;
        default:
            break;
        }
        painter->restore();
    } else {
        // 文件已就绪，停止动画
        const_cast<ChatMessageDelegate*>(this)->stopAnimationForMessage(message.message_idValue());
    }

    // 额外处理：文件不存在但状态不是未下载（例如曾经下载完成但文件被手动删除）-> 显示“文件已过期”
    if (!fileExists && effectiveStatus != DownloadStatus::NOT_DOWNLOADED &&
        effectiveStatus != DownloadStatus::DOWNLOADING) {
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 128));
        painter->drawRoundedRect(fileBubbleRect, 5, 5);
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
        painter->restore();
    }

    // 绘制时间
    QRect timeRect(fileBubbleRect.left(), fileBubbleRect.bottom() + margin,
                   fileBubbleRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintVoiceMessage(QPainter *painter, const QStyleOptionViewItem &option,
                                            const Message &message, bool isOwn) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    const int bubblePadding = 12;
    const int minVoiceBubbleWidth = MIN_VOICE_BUBBLE_WIDTH;
    const int maxVoiceBubbleWidth = MAX_VOICE_BUBBLE_WIDTH;
    const int voiceBubbleHeight = VOICE_BUBBLE_HEIGHT;
    const int playButtonSize = PLAY_BUTTON_SIZE;
    const int waveformHeight = WAVEFORM_HEIGHT;

    bool isOwnMessage = isOwn;
    int duration = message.durationValue();

    QRect avatarRect = getAvatarRect(option, isOwnMessage);
    paintAvatar(painter, avatarRect, message);

    int voiceBubbleWidth = qMin(maxVoiceBubbleWidth,
                                minVoiceBubbleWidth + duration * 3);

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

    painter->setPen(Qt::NoPen);
    if (isOwnMessage) {
        painter->setBrush(QColor(0x07, 0xC1, 0x60));
    } else {
        painter->setBrush(Qt::white);
    }
    painter->drawRoundedRect(voiceBubbleRect, 5, 5);

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
    bool isPlaying = player->isPlaying() && (player->currentMessageId() == message.message_id());

    paintPlayButtonAndWaveform(painter, voiceBubbleRect, isOwnMessage, isPlaying, message.message_idValue());
    paintDurationText(painter, voiceBubbleRect, duration, isOwnMessage);

    QRect timeRect(voiceBubbleRect.left(), voiceBubbleRect.bottom() + margin,
                   voiceBubbleRect.width(), 0);
    paintTime(painter, timeRect, option, message, isOwnMessage);

    painter->restore();
}

void ChatMessageDelegate::paintAvatar(QPainter *painter, const QRect &avatarRect,
                                      const Message &message) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QString nickname;
    if (m_contactController)
        nickname = m_contactController->getContactFromModel(message.sender_idValue()).user.nicknameValue();

    QPixmap avatarPixmap = thumbnailManager->getThumbnail(message.avatar(),
                                                          avatarRect.size(),
                                                          MediaType::Avatar, 5,
                                                          "", nickname);

    if (!avatarPixmap.isNull()) {
        painter->drawPixmap(avatarRect, avatarPixmap);
    } else {
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
        QString initial = message.senderName().isEmpty() ? "U" : message.senderName().left(1);
        painter->drawText(avatarRect, Qt::AlignCenter, initial.toUpper());
    }

    painter->restore();
}

void ChatMessageDelegate::paintTime(QPainter *painter, const QRect &rect, const QStyleOptionViewItem &option,
                                    const Message &message, bool isOwnMessage) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QString timeText = FormatTime(message.msg_timeValue());
    QFont timeFont = option.font;
    timeFont.setPointSizeF(7.5);
    painter->setFont(timeFont);
    QFontMetrics timeMetrics(timeFont);
    int timeHeight = timeMetrics.height();
    painter->setPen(QColor(150, 150, 150));
    QRect timeRect(rect.left(), rect.top(), rect.width(), timeHeight);

    if (isOwnMessage)
        painter->drawText(timeRect, Qt::AlignRight | Qt::AlignTop, timeText);
    else
        painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignTop, timeText);

    painter->restore();
}

void ChatMessageDelegate::paintFileIcon(QPainter *painter, const QRect &fileRect,
                                        const QString &extension) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QString typeText;
    bool unknownType = false;
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

    int foldSize = qMin(fileRect.width(), fileRect.height()) * 0.3;

    QPainterPath filePath;
    filePath.moveTo(fileRect.topLeft());
    filePath.lineTo(fileRect.topRight().x() - foldSize, fileRect.top());
    filePath.lineTo(fileRect.topRight().x(), fileRect.top() + foldSize);
    filePath.lineTo(fileRect.bottomRight());
    filePath.lineTo(fileRect.bottomLeft());
    filePath.closeSubpath();

    painter->setPen(QPen(iconColor, 1));
    painter->setBrush(iconColor);
    painter->drawPath(filePath);

    QColor foldedColor = unknownType ? iconColor.lighter(80) : iconColor.lighter(140);
    painter->setPen(foldedColor);
    painter->setBrush(foldedColor);
    QPainterPath foldedPath;
    foldedPath.moveTo(filePath.elementAt(1));
    foldedPath.lineTo(filePath.elementAt(2));
    foldedPath.lineTo(filePath.elementAt(1).x, filePath.elementAt(2).y);
    foldedPath.closeSubpath();
    painter->drawPath(foldedPath);

    painter->setPen(unknownType ? QColor(86, 106, 148) : Qt::white);
    QFont iconFont = painter->font();
    iconFont.setPointSize(typeText.length() <= 2 ? 12 : 8);
    painter->setFont(iconFont);
    QFontMetrics metrics(iconFont);
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
                                                     qint64 messageId) const
{
    const int playButtonSize = PLAY_BUTTON_SIZE;

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

    painter->setPen(Qt::NoPen);
    if (isOwnMessage)
        painter->setBrush(Qt::white);
    else
        painter->setBrush(QColor(0x07, 0xC1, 0x60));
    painter->drawEllipse(playButtonRect);

    painter->setBrush(isOwnMessage ? QColor(0x07, 0xC1, 0x60) : Qt::white);
    if (isPlaying) {
        int barWidth = 3;
        int barHeight = 10;
        int centerX = playButtonRect.center().x();
        int centerY = playButtonRect.center().y();
        QRect leftBar(centerX - barWidth - 2, centerY - barHeight/2, barWidth, barHeight);
        QRect rightBar(centerX + 2, centerY - barHeight/2, barWidth, barHeight);
        painter->drawRect(leftBar);
        painter->drawRect(rightBar);
    } else {
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

    // 波形区域（这里留空，实际由 paintVoiceWaveform 绘制，但为了方便，可以整合到此处，但为避免混乱，保留单独调用）
}

void ChatMessageDelegate::paintVoiceWaveform(QPainter *painter,
                                             const QRect &rect,
                                             bool isOwnMessage,
                                             bool isPlaying,
                                             qint64 messageId) const
{
    QList<int> heights;
    if (isPlaying) {
        heights = AudioPlayer::instance()->currentWaveform();
    } else {
        heights = {8, 12, 16, 12};
    }

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
        painter->setBrush(isOwnMessage ? Qt::white : QColor(0x07, 0xC1, 0x60));
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

void ChatMessageDelegate::paintMediaOverlay(QPainter *painter, const QRect &imageRect,
                                            DownloadStatus thumbStatus,
                                            DownloadStatus fileStatus,
                                            qint64 messageId) const
{
    // 决定覆盖层类型
    enum OverlayType { None, Download, Loading, Failed };
    OverlayType overlay = None;

    if (thumbStatus != DownloadStatus::COMPLETED) {
        // 缩略图自身未完成：显示加载或失败
        if (thumbStatus == DownloadStatus::DOWNLOADING)
            overlay = Loading;
        else if (thumbStatus == DownloadStatus::FAILED)
            overlay = Failed;
        // NOT_DOWNLOADED 理论上不会出现，因为自动下载，但也当作加载
        else
            overlay = Loading;
    } else {
        // 缩略图已完成，根据原文件状态决定
        switch (fileStatus) {
        case DownloadStatus::NOT_DOWNLOADED:
            overlay = Download;
            break;
        case DownloadStatus::DOWNLOADING:
            overlay = Loading;
            break;
        case DownloadStatus::FAILED:
            overlay = Failed;
            break;
        default:
            overlay = None;
            break;
        }
    }

    if (overlay == None) {
        const_cast<ChatMessageDelegate*>(this)->stopAnimationForMessage(messageId);
        return;
    }

    int iconSize = qMin(40, qMin(imageRect.width(), imageRect.height()) / 2);
    QRect iconRect(imageRect.center().x() - iconSize/2,
                   imageRect.center().y() - iconSize/2,
                   iconSize, iconSize);

    // 半透明背景圆
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 150));
    painter->drawEllipse(iconRect);

    painter->setPen(Qt::white);
    QFont iconFont = painter->font();
    iconFont.setPointSize(iconSize * 0.4);
    painter->setFont(iconFont);

    if (overlay == Download) {
        painter->drawText(iconRect, Qt::AlignCenter, "↓");
        const_cast<ChatMessageDelegate*>(this)->stopAnimationForMessage(messageId);
    } else if (overlay == Failed) {
        painter->drawText(iconRect, Qt::AlignCenter, "✗");
        const_cast<ChatMessageDelegate*>(this)->stopAnimationForMessage(messageId);
    } else if (overlay == Loading) {
        const_cast<ChatMessageDelegate*>(this)->startAnimationForMessage(messageId);
        int angle = m_animationAngles.value(messageId, 0);
        int arcSpan = 40 * 16; // 40度 * 16
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Qt::white, 3));
        painter->drawArc(iconRect, angle * 16, arcSpan);
    }
}

QSize ChatMessageDelegate::calculateTextSize(const QString &text, const QFont &font,
                                             int maxWidth) const
{
    if (text.isEmpty()) return QSize(0, 0);
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(QRect(0, 0, maxWidth, 0),
                                          Qt::TextWordWrap | Qt::AlignLeft, text);
    return textRect.size();
}

QRect ChatMessageDelegate::getClickableRect(const QStyleOptionViewItem &option,
                                            const Message &message, bool isOwn) const
{
    const int avatarSize = AVATAR_SIZE;
    const int margin = MARGIN;
    bool isOwnMessage = isOwn;

    switch (static_cast<MessageType>(message.typeValue())) {
    case MessageType::IMAGE:
    case MessageType::VIDEO: {
        const int avatarSize = AVATAR_SIZE;
        const int margin = MARGIN;

        auto thumbStatus = message.thumbDownloadStatus();
        QSize mediaSize(150, 150);
        if (thumbStatus == DownloadStatus::COMPLETED) {
            QPixmap thumb = thumbnailManager->getThumbnail(message.file_pathValue(), QSize(200, 300),
                                                           MediaType::ImageThumb, 0, message.thumbnail_pathValue());
            mediaSize = thumb.size();
            if (mediaSize.isEmpty()) mediaSize = QSize(150, 150);
        }

        if (isOwn) {
            return QRect(option.rect.right() - avatarSize - mediaSize.width() - 2 * margin,
                         option.rect.top() + margin,
                         mediaSize.width(), mediaSize.height());
        } else {
            return QRect(option.rect.left() + avatarSize + 2 * margin,
                         option.rect.top() + margin,
                         mediaSize.width(), mediaSize.height());
        }
    }
    case MessageType::FILE: {
        int fileBubbleWidth = FILE_BUBBLE_WIDTH;
        int fileBubbleHeight = FILE_BUBBLE_HEIGHT;
        if (isOwnMessage) {
            return QRect(option.rect.right() - 2 * margin - avatarSize - fileBubbleWidth,
                         option.rect.top() + margin, fileBubbleWidth, fileBubbleHeight);
        } else {
            return QRect(option.rect.left() + 2 * margin + avatarSize,
                         option.rect.top() + margin, fileBubbleWidth, fileBubbleHeight);
        }
    }
    case MessageType::VOICE: {
        int minWidth = MIN_VOICE_BUBBLE_WIDTH;
        int maxWidth = MAX_VOICE_BUBBLE_WIDTH;
        int height = VOICE_BUBBLE_HEIGHT;
        int duration = message.durationValue();
        int width = qMin(maxWidth, minWidth + duration * 3);
        if (isOwnMessage) {
            return QRect(option.rect.right() - avatarSize - 2 * margin - width,
                         option.rect.top() + margin + (avatarSize - height) / 2,
                         width, height);
        } else {
            return QRect(option.rect.left() + avatarSize + 2 * margin,
                         option.rect.top() + margin + (avatarSize - height) / 2,
                         width, height);
        }
    }
    case MessageType::TEXT: {
        int maxBubbleWidth = option.rect.width() * 0.6;
        int bubblePadding = BUBBLE_PADDING;
        QFont contentFont;
        contentFont.setPointSizeF(10.5);
        contentFont.setFamily(QStringLiteral("微软雅黑"));
        QSize textSize = calculateTextSize(message.contentValue(), contentFont, maxBubbleWidth - 2 * bubblePadding);
        int bubbleWidth = qMin(textSize.width() + 2 * bubblePadding, maxBubbleWidth);
        int bubbleHeight = textSize.height() + 2 * bubblePadding;
        if (isOwnMessage) {
            return QRect(option.rect.right() - margin - avatarSize - margin - bubbleWidth,
                         option.rect.top() + margin, bubbleWidth, bubbleHeight);
        } else {
            return QRect(option.rect.left() + margin + avatarSize + margin,
                         option.rect.top() + margin, bubbleWidth, bubbleHeight);
        }
    }
    default:
        return QRect();
    }
}

void ChatMessageDelegate::onMediaLoaded(const QString &resourcePath, const QPixmap &media, MediaType type)
{
    if (ChatMessageListView *view = qobject_cast<ChatMessageListView *>(parent())) {
        view->viewport()->update();
        QAbstractItemModel *model = view->model();
        if (model) {
            QModelIndex topLeft = model->index(0, 0);
            QModelIndex bottomRight = model->index(model->rowCount() - 1, 0);
            emit model->dataChanged(topLeft, bottomRight);
        }
    }
}

void ChatMessageDelegate::startAnimationForMessage(qint64 messageId)
{
    if (m_animationTimers.contains(messageId))
        return;
    int timerId = startTimer(ANIMATION_INTERVAL);
    m_animationTimers[messageId] = timerId;
    m_animationAngles[messageId] = 0;
}

void ChatMessageDelegate::stopAnimationForMessage(qint64 messageId)
{
    if (m_animationTimers.contains(messageId)) {
        killTimer(m_animationTimers[messageId]);
        m_animationTimers.remove(messageId);
        m_animationAngles.remove(messageId);
    }
}

void ChatMessageDelegate::timerEvent(QTimerEvent *event)
{
    QMap<qint64, int>::iterator it = m_animationTimers.begin();
    while (it != m_animationTimers.end()) {
        if (it.value() == event->timerId()) {
            qint64 msgId = it.key();
            m_animationAngles[msgId] = (m_animationAngles[msgId] + ARC_STEP) % 360;
            // 刷新视图
            if (ChatMessageListView *view = qobject_cast<ChatMessageListView *>(parent())) {
                view->viewport()->update();
            }
            break;
        }
        ++it;
    }
    QStyledItemDelegate::timerEvent(event);
}