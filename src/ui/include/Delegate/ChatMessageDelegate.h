#ifndef CHATMESSAGEDELEGATE_H
#define CHATMESSAGEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include "ThumbnailResourceManager.h"
#include "Message.h"


class ChatMessageDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ChatMessageDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override;

signals:
    void mediaClicked(const qint64 &msgId, const qint64 &conversationId);
    void fileClicked(const QString &filePath);
    void voiceClicked(const QString &voicePath, const qint64 &messageId);
    void textClicked(const QString &text);
    void rightClicked(const QPoint& globalPos, const Message &message);



private slots:
    void onMediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type);


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
                                    const qint64 &messageId) const;
    void paintVoiceWaveform(QPainter *painter,
                            const QRect &rect,
                            bool isOwnMessage,
                            bool isPlaying,
                            const qint64 &messageId) const;
    void paintDurationText(QPainter *painter, const QRect &bubbleRect,
                           int duration, bool isOwnMessage) const;

    // 工具方法
    QSize calculateTextSize(const QString &text, const QFont &font, int maxWidth) const;
    QRect getClickableRect(const QStyleOptionViewItem &option, const Message &message,
                           const bool &isOwn) const;
    bool handleLeftClick(QMouseEvent *mouseEvent, const QStyleOptionViewItem &option,
                         const QModelIndex &index);

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

    ThumbnailResourceManager *thumbnailManager;
};

#endif // CHATMESSAGEDELEGATE_H
