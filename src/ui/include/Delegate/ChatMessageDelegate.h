#ifndef CHATMESSAGEDELEGATE_H
#define CHATMESSAGEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QMap>
#include "ThumbnailResourceManager.h"
#include "Message.h"

class ContactController;

class ChatMessageDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ChatMessageDelegate(QObject *parent = nullptr);
    ~ChatMessageDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override;

    void setContactController(ContactController *contactController) { m_contactController = contactController; }

signals:
    void mediaClicked(qint64 msgId, qint64 conversationId);
    void fileClicked(const QString &filePath);
    void voiceClicked(const QString &voicePath, qint64 messageId);
    void textClicked(const QString &text);
    void rightClicked(const QPoint &globalPos, const Message &message);
    void avatarClicked(qint64 senderId);
    void downloadRequested(qint64 messageId);  // 请求下载原文件

private slots:
    void onMediaLoaded(const QString &resourcePath, const QPixmap &media, MediaType type);

private:
    // 不同类型消息的绘制方法
    void paintTextMessage(QPainter *painter, const QStyleOptionViewItem &option,
                          const Message &message, bool isOwn) const;
    void paintImageMessage(QPainter *painter, const QStyleOptionViewItem &option,
                           const Message &message, bool isOwn) const;
    void paintVideoMessage(QPainter *painter, const QStyleOptionViewItem &option,
                           const Message &message, bool isOwn) const;
    void paintFileMessage(QPainter *painter, const QStyleOptionViewItem &option,
                          const Message &message, bool isOwn) const;
    void paintVoiceMessage(QPainter *painter, const QStyleOptionViewItem &option,
                           const Message &message, bool isOwn) const;

    // 辅助绘制方法
    void paintAvatar(QPainter *painter, const QRect &avatarRect,
                     const Message &message) const;
    void paintTime(QPainter *painter, const QRect &rect, const QStyleOptionViewItem &option,
                   const Message &message, bool isOwnMessage) const;
    void paintFileIcon(QPainter *painter, const QRect &fileRect,
                       const QString &extension) const;
    void paintPlayButtonAndWaveform(QPainter *painter,
                                    const QRect &bubbleRect,
                                    bool isOwnMessage,
                                    bool isPlaying,
                                    qint64 messageId) const;
    void paintVoiceWaveform(QPainter *painter,
                            const QRect &rect,
                            bool isOwnMessage,
                            bool isPlaying,
                            qint64 messageId) const;
    void paintDurationText(QPainter *painter, const QRect &bubbleRect,
                           int duration, bool isOwnMessage) const;

    // 新增：绘制媒体覆盖层（下载/加载/失败）
    void paintMediaOverlay(QPainter *painter, const QRect &imageRect,
                           DownloadStatus thumbStatus,
                           DownloadStatus fileStatus,
                           qint64 messageId) const;

    // 工具方法
    QSize calculateTextSize(const QString &text, const QFont &font, int maxWidth) const;
    QRect getClickableRect(const QStyleOptionViewItem &option, const Message &message,
                           bool isOwn) const;
    bool handleLeftClick(QMouseEvent *mouseEvent, const QStyleOptionViewItem &option,
                         const QModelIndex &index);
    QRect getAvatarRect(const QStyleOptionViewItem &option, bool isOwn) const;

    // 动画支持
    void startAnimationForMessage(qint64 messageId);
    void stopAnimationForMessage(qint64 messageId);
    void timerEvent(QTimerEvent *event) override;

private:
    // 常量定义
    static const int AVATAR_SIZE = 38;
    static const int MARGIN = 10;
    static const int BUBBLE_PADDING = 10;
    static const int TIME_SPACING = 10;
    static const int FILE_BUBBLE_WIDTH = 240;
    static const int FILE_BUBBLE_HEIGHT = 95;
    static const int MIN_VOICE_BUBBLE_WIDTH = 90;
    static const int MAX_VOICE_BUBBLE_WIDTH = 200;
    static const int VOICE_BUBBLE_HEIGHT = 40;
    static const int PLAY_BUTTON_SIZE = 24;
    static const int WAVEFORM_HEIGHT = 16;
    static const int ICON_WIDTH = 29;
    static const int ICON_HEIGHT = 40;

    static const int ANIMATION_INTERVAL = 50;   // 动画间隔（毫秒）
    static const int ARC_STEP = 10;             // 每帧旋转角度

    ThumbnailResourceManager *thumbnailManager;
    ContactController *m_contactController;

    // 动画状态（mutable 因为要在 const 方法中修改）
    mutable QMap<qint64, int> m_animationAngles;   // 消息ID -> 当前角度
    mutable QMap<qint64, int> m_animationTimers;   // 消息ID -> timerId
};

#endif // CHATMESSAGEDELEGATE_H