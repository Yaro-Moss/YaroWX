#ifndef MESSAGECONTROLLER_H
#define MESSAGECONTROLLER_H

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include "Contact.h"
#include "Conversation.h"
#include "MediaItem.h"
#include "Message.h"
#include "ChatMessagesModel.h"

class ImageProcessor;
class ChatMessagesModel;
class FileCopyProcessor;
class VideoProcessor;

class MessageController : public QObject
{
    Q_OBJECT
public:
    explicit MessageController(QObject* parent = nullptr);
    ~MessageController();

    // 模型访问
    ChatMessagesModel* messagesModel() const { return m_messagesModel; }

    // 设置当前会话和用户
    void setCurrentConversation(Conversation conversation);
    void setCurrentLoginUser(const Contact &user);

    // 发送消息（UI 调用）
    Q_INVOKABLE void sendTextMessage(const QString& content);
    Q_INVOKABLE void sendVoiceMessage(const QString& filePath, int duration);
    Q_INVOKABLE void preprocessImageBeforeSend(QStringList pathLis);
    Q_INVOKABLE void preprocessVideoBeforeSend(QStringList fileList);
    Q_INVOKABLE void preprocessFileBeforeSend(QStringList fileList);

    // 加载消息
    Q_INVOKABLE void loadRecentMessages(int limit = 20);
    Q_INVOKABLE void loadMoreMessages(int limit = 20);
    Q_INVOKABLE void getMediaItems(qint64 conversationId);

    // 菜单操作
    Q_INVOKABLE void handleCopy(const Message &message);
    Q_INVOKABLE void handleZoom();
    Q_INVOKABLE void handleTranslate();
    Q_INVOKABLE void handleSearch();
    Q_INVOKABLE void handleForward();
    Q_INVOKABLE void handleFavorite();
    Q_INVOKABLE void handleRemind();
    Q_INVOKABLE void handleMultiSelect();
    Q_INVOKABLE void handleQuote();
    Q_INVOKABLE void handleDelete(const Message &message);

signals:
    void send(const QVector<Message>& recentMessages);  // 临时测试
    void mediaItemsLoaded(const QList<MediaItem>& items);
    void messageSaved();

private slots:

    void sendImageMessage(const qint64 conversationId, const QString &originalImagePath, const QString &thumbnailPath, bool success);
    void sendVideoMessage(qint64 conversationId, const QString &originalPath, const QString &thumbnailPath, bool success);
    void sendFileMessage(const qint64 conversationId, bool success, const QString &sourcePath, const QString &targetPath, const QString &errorMessage);

private:
    // 辅助方法
    Message createMessage(const Conversation &conversation, MessageType type,
                          const QString& content = QString(),
                          const QString& filePath = QString(),
                          qint64 fileSize = 0,
                          int duration = 0,
                          const QString& thumbnailPath = QString());
    void connectSignals();
    void saveMessage(const Message& msg);  // 异步保存到数据库
    qint64 generateUniqueMessageId();

    ChatMessagesModel* m_messagesModel;
    Conversation m_currentConversation;
    Contact m_currentLoginUser;
    int currentOffset;  // 已加载的消息数量

    ImageProcessor* imageProcessor;
    FileCopyProcessor* fileCopyProcessor;
    VideoProcessor* videoProcessor;
};

#endif // MESSAGECONTROLLER_H
