#include "MessageController.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QAtomicInteger>
#include "ImageProcessor.h"
#include "FileCopyProcessor.h"
#include "MessageDao.h"
#include "VideoProcessor.h"
#include "ORM.h"
#include "ChatMessagesModel.h"


MessageController::MessageController(QObject* parent)
    : QObject(parent)
    , m_messagesModel(new ChatMessagesModel(this))
    , currentOffset(0)
    , imageProcessor(new ImageProcessor(this))
    , fileCopyProcessor(new FileCopyProcessor(this))
    , videoProcessor(new VideoProcessor(this))
{
    connectSignals();
}

MessageController::~MessageController()
{
}

void MessageController::connectSignals()
{
    if(imageProcessor){
        connect(imageProcessor, &ImageProcessor::imageProcessed, this, &MessageController::sendImageMessage);
    }
    if(fileCopyProcessor){
        connect(fileCopyProcessor, &FileCopyProcessor::fileCopied, this, &MessageController::sendFileMessage);
    }
    if(videoProcessor){
        connect(videoProcessor, &VideoProcessor::videoProcessed, this, &MessageController::sendVideoMessage);
    }
}

void MessageController::setCurrentConversation(Conversation conversation)
{
    m_currentConversation = conversation;
    m_messagesModel->setConversationId(conversation.conversation_idValue());
    currentOffset = 0;
    loadRecentMessages();  // 切换会话时自动加载最近消息
}

void MessageController::setCurrentLoginUser(const Contact &user){
    m_currentLoginUser = user;
    m_messagesModel->setCurrentLoginUser(m_currentLoginUser);
}

// 生成全局唯一消息 ID（线程安全）
qint64 MessageController::generateUniqueMessageId()
{
    static QAtomicInteger<quint64> s_counter = 0;
    quint64 base = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    quint64 unique = (base << 20) | (++s_counter & 0xFFFFF);
    return static_cast<qint64>(unique);
}

// 异步保存消息到数据库
void MessageController::saveMessage(const Message& msg) {

    QFutureWatcher<QPair<int, bool>>* watcher = new QFutureWatcher<QPair<int, bool>>(this);
    connect(watcher, &QFutureWatcher<QPair<int, bool>>::finished, this, [this, watcher]() {
        auto result = watcher->future().result();
        emit messageSaved();
        watcher->deleteLater();
    });
    auto future = QtConcurrent::run([msg]() -> QPair<int, bool> {
        Orm orm;
        Message copy = msg;
        bool success = orm.insert(copy);
        return {copy.message_idValue(), success};
    });

    watcher->setFuture(future);
}

// 发送文本消息
void MessageController::sendTextMessage(const QString& content)
{
    if (content.trimmed().isEmpty() || !m_currentConversation.isValid()) {
        return;
    }

    Message message = createMessage(m_currentConversation, MessageType::TEXT, content);

    m_messagesModel->addMessage(message);
    currentOffset += 1;

    saveMessage(message);

    QVector<Message> recent = m_messagesModel->getRecentMessages(9);
    emit send(recent);
}

void MessageController::preprocessImageBeforeSend(QStringList pathLis)
{
    imageProcessor->processImages(m_currentConversation.conversation_idValue(), pathLis);
}
// 图片消息发送回调
void MessageController::sendImageMessage(const qint64 conversationId,
                                         const QString &originalImagePath,
                                         const QString &thumbnailPath,
                                         bool success)
{
    if(!success) return;

    QFileInfo fileInfo(originalImagePath);
    qint64 fileSize = fileInfo.size();
    Conversation tempConv = m_currentConversation;
    tempConv.setconversation_id(conversationId);

    Message message = createMessage(tempConv,
                                    MessageType::IMAGE,
                                    "【图片】"+fileInfo.fileName(),
                                    originalImagePath,
                                    fileSize,
                                    0,
                                    thumbnailPath);

    if(conversationId == m_currentConversation.conversation_id()){
        m_messagesModel->addMessage(message);
        currentOffset += 1;
    }

    saveMessage(message);

    QVector<Message> recent = m_messagesModel->getRecentMessages(9);
    emit send(recent);
}

void MessageController::preprocessVideoBeforeSend(QStringList fileList)
{
    videoProcessor->processVideos(m_currentConversation.conversation_idValue(), fileList);
}
// 视频消息发送回调
void MessageController::sendVideoMessage(qint64 conversationId,
                                         const QString &originalPath,
                                         const QString &thumbnailPath,
                                         bool success)
{
    QFileInfo fileInfo(originalPath);
    qint64 fileSize = fileInfo.size();
    Conversation tempConv = m_currentConversation;
    tempConv.setconversation_id(conversationId);

    Message message = createMessage(tempConv,
                                    MessageType::VIDEO,
                                    "【视频】"+fileInfo.fileName(),
                                    originalPath,
                                    fileSize,
                                    0,
                                    thumbnailPath);

    if(conversationId == m_currentConversation.conversation_id()){
        m_messagesModel->addMessage(message);
        currentOffset += 1;
    }

    saveMessage(message);

    QVector<Message> recent = m_messagesModel->getRecentMessages(9);
    emit send(recent);
}

void MessageController::preprocessFileBeforeSend(QStringList fileList)
{
    fileCopyProcessor->copyFiles(m_currentConversation.conversation_idValue(), fileList);
}
// 文件消息发送回调
void MessageController::sendFileMessage(const qint64 conversationId,
                                        bool success,
                                        const QString &sourcePath,
                                        const QString &targetPath,
                                        const QString &errorMessage)
{
    Q_UNUSED(sourcePath)
    if (!success) {
        qWarning() << "File copy failed:" << errorMessage;
        return;
    }

    QFileInfo fileInfo(targetPath);
    qint64 fileSize = fileInfo.size();
    Conversation tempConv = m_currentConversation;
    tempConv.setconversation_id(conversationId);

    Message message = createMessage(tempConv,
                                    MessageType::FILE,
                                    "【文件】"+fileInfo.fileName(),
                                    targetPath,
                                    fileSize);

    if(conversationId == m_currentConversation.conversation_id()){
        m_messagesModel->addMessage(message);
        currentOffset += 1;
    }

    saveMessage(message);

    QVector<Message> recent = m_messagesModel->getRecentMessages(9);
    emit send(recent);
}

// 语音消息发送
void MessageController::sendVoiceMessage(const QString& filePath, int duration)
{
    if (filePath.isEmpty() || !m_currentConversation.isValid()) {
        return;
    }

    QFileInfo fileInfo(filePath);
    qint64 fileSize = fileInfo.size();
    Message message = createMessage(m_currentConversation,
                                    MessageType::VOICE,
                                    "语音消息",
                                    filePath,
                                    fileSize,
                                    duration);

    m_messagesModel->addMessage(message);
    currentOffset += 1;

    saveMessage(message);

    QVector<Message> recent = m_messagesModel->getRecentMessages(9);
    emit send(recent);
}

// 加载最近消息（清空模型，重新加载最新 limit 条）
void MessageController::loadRecentMessages(int limit)
{
    if (!m_currentConversation.isValid()) return;
    qint64 convId = m_currentConversation.conversation_idValue();

    QFutureWatcher<QList<Message>> *watcher = new QFutureWatcher<QList<Message>>(this);
    connect(watcher, &QFutureWatcher<QList<Message>>::finished, this, [this, watcher]() {
        QList<Message> messages = watcher->result();
        std::reverse(messages.begin(), messages.end()); // 转为升序
        m_messagesModel->clearAll();
        for (const Message &msg : messages) {
            m_messagesModel->addMessage(msg);
        }
        currentOffset = messages.size();
        watcher->deleteLater();
    });

    QFuture<QList<Message>> future = QtConcurrent::run([convId, limit]() -> QList<Message> {
        MessageDao dao;  // 在后台线程创建临时 dao 对象
        return dao.fetchMessages(convId, limit, 0);
    });
    watcher->setFuture(future);
}

// 加载更多历史消息（分页，offset = currentOffset）
void MessageController::loadMoreMessages(int limit)
{
    if (!m_currentConversation.isValid()) return;
    qint64 convId = m_currentConversation.conversation_idValue();
    int offset = currentOffset;

    QFutureWatcher<QList<Message>> *watcher = new QFutureWatcher<QList<Message>>(this);
    connect(watcher, &QFutureWatcher<QList<Message>>::finished, this, [this, watcher]() {
        QList<Message> messages = watcher->result();
        if (messages.isEmpty()) {
            watcher->deleteLater();
            return;
        }
        for (const Message &msg : std::as_const(messages)) {
            m_messagesModel->addMessage(msg);
        }
        currentOffset += messages.size();
        watcher->deleteLater();
    });

    QFuture<QList<Message>> future = QtConcurrent::run([convId, limit, offset]() -> QList<Message> {
        MessageDao dao;
        return dao.fetchMessages(convId, limit, offset);
    });
    watcher->setFuture(future);
}

// 获取会话的所有媒体
void MessageController::getMediaItems(qint64 conversationId)
{
    QFutureWatcher<QList<MediaItem>> *watcher = new QFutureWatcher<QList<MediaItem>>(this);
    connect(watcher, &QFutureWatcher<QList<MediaItem>>::finished, this, [this, watcher]() {
        QList<MediaItem> items = watcher->result();
        emit mediaItemsLoaded(items);
        watcher->deleteLater();
    });

    QFuture<QList<MediaItem>> future = QtConcurrent::run([conversationId]() -> QList<MediaItem> {
        MessageDao dao;
        return dao.fetchMediaItems(conversationId);
    });
    watcher->setFuture(future);
}

// 菜单操作
void MessageController::handleCopy(const Message &message)
{
    if(message.isText()){
        QApplication::clipboard()->setText(message.contentValue());
    }
    else if(message.isMedia() || message.isFile() || message.isImage())
    {
        QList<QUrl> urls;
        urls.append(QUrl::fromLocalFile(message.file_pathValue()));

        QMimeData *mimeData = new QMimeData;
        mimeData->setUrls(urls);
        QApplication::clipboard()->setMimeData(mimeData);
    }
}

void MessageController::handleDelete(const Message &message)
{
    // 从模型中移除
    m_messagesModel->removeMessageById(message.message_idValue());
    currentOffset = qMax(0, currentOffset - 1);  // 减少计数

    // 异步删除数据库记录和文件
    QtConcurrent::run([message]() {
        Orm orm;
        auto msgOpt = orm.findById<Message>(message.message_idValue());
        if (msgOpt) {
            orm.remove(*msgOpt);
            QFile file(message.file_pathValue());
            QFile thumb(message.thumbnail_pathValue());
            file.remove();
            thumb.remove();
        }
    });
}

// 其他菜单操作（空实现）
void MessageController::handleZoom() { }
void MessageController::handleTranslate() { }
void MessageController::handleSearch() { }
void MessageController::handleForward() { }
void MessageController::handleFavorite() { }
void MessageController::handleRemind() { }
void MessageController::handleMultiSelect() { }
void MessageController::handleQuote() { }

// 辅助方法：创建消息对象
Message MessageController::createMessage(const Conversation &conversation,
                                         MessageType type,
                                         const QString& content,
                                         const QString& filePath,
                                         qint64 fileSize,
                                         int duration,
                                         const QString& thumbnailPath)
{
    Message message;
    message.setmessage_id(generateUniqueMessageId());   // 设置唯一 ID
    message.setconversation_id(conversation.conversation_id());
    message.setsender_id(m_currentLoginUser.user_id());
    message.setconsignee_id(conversation.targetId());
    message.setSenderName(m_currentLoginUser.user.nicknameValue());
    message.setAvatar(m_currentLoginUser.user.avatar_local_pathValue());

    message.settype(static_cast<int>(type));
    message.setcontent(content);
    message.setfile_path(filePath);
    message.setfile_size(fileSize);
    message.setduration(duration);
    message.setthumbnail_path(thumbnailPath);
    message.setmsg_time(QDateTime::currentSecsSinceEpoch());

    return message;
}
