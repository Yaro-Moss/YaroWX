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
#include "ConfigManager.h"
#include "ContactController.h"
#include "ConversationController.h"
#include "ImageProcessor.h"
#include "FileCopyProcessor.h"
#include "MessageDao.h"
#include "VideoProcessor.h"
#include "ORM.h"
#include "ChatMessagesModel.h"
#include "FileUploader.h"
#include "WebSocketManager.h"
#include "FileDownloader.h"


MessageController::MessageController(QObject* parent)
    : QObject(parent)
    , m_messagesModel(new ChatMessagesModel(this))
    , currentOffset(0)
    , imageProcessor(new ImageProcessor(this))
    , videoProcessor(new VideoProcessor(this))
    , fileCopyProcessor(new FileCopyProcessor(this))
{
    connectSignals();

    // 连接 WebSocket 信号
    WebSocketManager* ws = WebSocketManager::instance();
    connect(ws, &WebSocketManager::messageSendFailed, this, &MessageController::onMessageSendFailed);
    connect(ws, &WebSocketManager::newMessageReceived, this, &MessageController::onNewMessageReceived);
    connect(ws, &WebSocketManager::messageAcknowledged, this, &MessageController::onMessageAck);

}

MessageController::~MessageController()
{
}

void MessageController::setConversationController(ConversationController* controller)
{
    m_conversationController = controller;
}

void MessageController::setContactController(ContactController* contactController)
{
    m_contactController = contactController;
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

void MessageController::sendMediaMessageInternal(
    int msgType,
    const Conversation& conv,
    const QString& localFilePath,
    const QString& thumbnailPath,
    const QString& displayContent,
    qint64 fileSize,
    int duration)
{
    if (!conv.isValid()) return;

    // 创建临时消息
    Message tempMsg = createMessage(conv,
                                    static_cast<MessageType>(msgType),
                                    displayContent,
                                    localFilePath,
                                    fileSize,
                                    duration,
                                    thumbnailPath);
    tempMsg.setthumb_download_status(2);
    tempMsg.setfile_download_status(2);
    qint64 tempId = tempMsg.message_idValue();

    // 显示并暂存
    if (conv.conversation_id() == m_currentConversation.conversation_id()) {
        m_messagesModel->addMessage(tempMsg);
        currentOffset++;
    }
    m_pendingMessages[tempId] = tempMsg;
    // 上传文件
    FileUploader::instance()->uploadFile(localFilePath, thumbnailPath,
                                         [this, tempId, conv, msgType, duration, displayContent, fileSize]
                                         (bool ok, QString url, QString thumbUrl, QString error) {
                                             if (!ok) {
                                                 emit uploadFailed(conv.conversation_idValue(), tempId, error);
                                                 removePendingMessage(tempId, true);
                                                 return;
                                             }
                                             if (!m_pendingMessages.contains(tempId)) return;

                                             // 更新 pending 消息的 URL
                                             Message msg = m_pendingMessages.value(tempId);
                                             msg.setfile_url(url);
                                             m_pendingMessages[tempId] = msg;

                                             // 构造并发送 WebSocket JSON
                                             QJsonObject json;
                                             json["chat_type"] = conv.typeValue();
                                             json["target_id"] = conv.targetId().toLongLong();
                                             json["msg_type"] = msgType;
                                             json["content"] = displayContent;
                                             json["media_url"] = url;
                                             json["media_thumb_url"] = thumbUrl;
                                             json["media_size"] = fileSize;
                                             json["media_format"] = QFileInfo(url).suffix().toLower();
                                             json["media_duration"] = duration;
                                             json["client_temp_id"] = QString::number(tempId);

                                             WebSocketManager::instance()->sendTextMessage(QJsonDocument(json).toJson(QJsonDocument::Compact));
                                         });
}

// 发送文本消息
void MessageController::sendTextMessage(const QString& content)
{
    if (content.trimmed().isEmpty() || !m_currentConversation.isValid())
        return;

    Conversation conv = m_currentConversation;
    Message msg = createMessage(conv, MessageType::TEXT, content);

    // 文本消息直接通过 WebSocket 发送
    QJsonObject json;
    json["chat_type"] = conv.typeValue();
    json["target_id"] = conv.targetId().toLongLong();
    json["msg_type"] = 0; // 0:文本
    json["content"] = content;
    json["client_temp_id"] = QString::number(msg.message_idValue());

    // 先显示，待确认后再保存
    m_messagesModel->addMessage(msg);
    currentOffset++;

    qDebug()<<"content: "<<content;
    m_pendingMessages[msg.message_idValue()] = msg;
    WebSocketManager::instance()->sendTextMessage(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

// 图片发送
void MessageController::preprocessImageBeforeSend(QStringList pathLis)
{
    imageProcessor->processImages(m_currentConversation.conversation_idValue(), pathLis);
}
void MessageController::sendImageMessage(qint64 conversationId,
                                         const QString& originalPath,
                                         const QString& thumbnailPath,
                                         bool success)
{
    if (!success) return;
    Conversation conv = m_conversationController->getConversationFromModel(conversationId);
    if (!conv.isValid()) return;
    QFileInfo fi(originalPath);
    sendMediaMessageInternal(1, conv, originalPath, thumbnailPath,
                             "【图片】" + fi.fileName(), fi.size(), 0);
}

// 视频消息发送
void MessageController::preprocessVideoBeforeSend(QStringList fileList)
{
    videoProcessor->processVideos(m_currentConversation.conversation_idValue(), fileList);
}
void MessageController::sendVideoMessage(qint64 conversationId,
                                         const QString& originalPath,
                                         const QString& thumbnailPath,
                                         bool success)
{
    if (!success) return;
    Conversation conv = m_conversationController->getConversationFromModel(conversationId);
    if (!conv.isValid()) return;
    QFileInfo fi(originalPath);
    sendMediaMessageInternal(2, conv, originalPath, thumbnailPath,
                             "【视频】" + fi.fileName(), fi.size(), 0);
}

// 文件消息发送
void MessageController::preprocessFileBeforeSend(QStringList fileList)
{
    fileCopyProcessor->copyFiles(m_currentConversation.conversation_idValue(), fileList);
}
void MessageController::sendFileMessage(qint64 conversationId,
                                        bool success,
                                        const QString& sourcePath,
                                        const QString& targetPath,
                                        const QString& errorMessage)
{
    Q_UNUSED(sourcePath)
    if (!success) {
        qWarning() << "File copy failed:" << errorMessage;
        return;
    }
    Conversation conv = m_conversationController->getConversationFromModel(conversationId);
    if (!conv.isValid()) return;
    QFileInfo fi(targetPath);
    sendMediaMessageInternal(3, conv, targetPath, "",
                             "【文件】" + fi.fileName(), fi.size(), 0);
}

// 语音消息发送
void MessageController::sendVoiceMessage(const QString& filePath, int duration)
{
    if (filePath.isEmpty() || !m_currentConversation.isValid()) return;
    QFileInfo fi(filePath);
    sendMediaMessageInternal(4, m_currentConversation, filePath, "",
                             "语音消息", fi.size(), duration);
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


void MessageController::onNewMessageReceived(const QJsonObject& message)
{
    qDebug()<<"新消息："<<message;
    int chatType = message["chat_type"].toInt();
    qint64 targetId = message["target_id"].toVariant().toLongLong();
    int msgType = message["msg_type"].toInt();
    qint64 fromUserId = message["from_user_id"].toVariant().toLongLong();
    qint64 timestamp = message["timestamp"].toVariant().toLongLong();
    qint64 convTargetId = (chatType == 0) ? fromUserId : targetId;

    if (!m_conversationController) return;
    qint64 conversationId = m_conversationController->getConversationIdByTarget(chatType, convTargetId);

    auto handleMessageLogic = [=]() {
        // 获取最新的会话ID（创建成功后必须重新获取）
        qint64 realConvId = m_conversationController->getConversationIdByTarget(chatType, convTargetId);
        if (realConvId == -1) return;

        Message msg;
        msg.setconversation_id(realConvId);
        msg.setsender_id(fromUserId);
        msg.setconsignee_id(targetId);
        msg.settype(msgType);
        msg.setcontent(message["content"].toString());
        QString fileurl = ConfigManager::instance()->baseUrl()+message["media_url"].toString();
        msg.setfile_url(fileurl);
        msg.setfile_size(message["media_size"].toVariant().toLongLong());
        msg.setduration(message["media_duration"].toInt());
        QString thumurl = ConfigManager::instance()->baseUrl()+message["media_thumb_url"].toString();
        msg.setthumbnail_path(thumurl);
        msg.setmsg_time(timestamp);
        msg.setfile_path("null");

        msg.setmessage_id(generateUniqueMessageId());
        msg.setthumb_download_status(1);
        msg.setfile_download_status(0);

        saveMessage(msg);
        if (realConvId == m_currentConversation.conversation_id()) {
            m_messagesModel->addMessage(msg);
            currentOffset++;
        }

        if (msgType >= 1 && msgType <= 4) { // 文件类型
            FileDownloader* fileDownloader = new FileDownloader();
            QDir baseDir(ConfigManager::instance()->dataSavePath());
            QString dir;
            if(msgType==1)
                dir = baseDir.absoluteFilePath("image_thumbnails");
            else if (msgType==2)
                dir = baseDir.absoluteFilePath("video_thumbnails");

            QString name = FileDownloader::extractBaseFileName(QUrl(msg.thumbnail_pathValue()));
            QString path = FileDownloader::generateUniqueFilePath(dir, name);

            fileDownloader->download(QUrl(msg.thumbnail_pathValue()), path);
            connect(fileDownloader, &FileDownloader::finished, this,
                [msg,this,fileDownloader,path](bool success, const QString& localPath){
                Orm orm;
                auto msgOpt = orm.findById<Message>(msg.message_idValue());
                if (msgOpt) {
                    msgOpt->setthumbnail_path(localPath);
                    msgOpt->setthumb_download_status(2);
                    orm.update(*msgOpt);
                    m_messagesModel->updateMessage(*msgOpt);
                }
                fileDownloader->deleteLater();
            });

        }
        if(chatType==4){
            onDownloadRequested(msg.message_idValue());
        }
    };

    if (conversationId == -1) {
        // 会话不存在 → 创建会话，成功后执行消息处理
        connect(m_conversationController, &ConversationController::createChatSuccess, this,
                [=]() {
                    handleMessageLogic();
                },
                Qt::SingleShotConnection); // 防止重复连接

        // 创建单聊/群聊
        if (chatType == 0) {
            Contact cont = m_contactController->getContactFromModel(fromUserId);
            m_conversationController->createSingleChat(cont);
        } else {
            m_conversationController->createGroupChat(targetId);
        }
        return;
    }

    handleMessageLogic();
}

void MessageController::onMessageSendFailed(qint64 localId, const QString& error)
{
    removePendingMessage(localId, true);
    emit messageSendFailed(localId, error);
}

void MessageController::onMessageAck(qint64 localId, bool success)
{
    if (!m_pendingMessages.contains(localId)) return;
    if(success){
        Message msg = m_pendingMessages.take(localId);
        saveMessage(msg);
    }else{
        removePendingMessage(localId, true);
    }
}

void MessageController::removePendingMessage(qint64 localId, bool removeFromModel)
{
    if (!m_pendingMessages.contains(localId)) return;
    Message msg = m_pendingMessages.take(localId);
    if (removeFromModel && msg.conversation_id() == m_currentConversation.conversation_id()) {
        qDebug()<<"MessageController::removePendingMessage: 发送失败删除信息";
        m_messagesModel->removeMessageById(localId);
        currentOffset--;
    }
}

void MessageController::updateMessageLocalPath(qint64 msgId, const QString& localPath)
{
    QtConcurrent::run([msgId, localPath]() {
        Orm orm;
        auto msgOpt = orm.findById<Message>(msgId);
        if (msgOpt) {
            msgOpt->setfile_path(localPath);
            orm.update(*msgOpt);
        }
    });
}

void MessageController::onDownloadRequested(qint64 messageId)
{
    Message msg = m_messagesModel->getMessageById(messageId);
    msg.setfile_download_status(1);
    m_messagesModel->updateMessage(msg);
    Orm orm;
    orm.update(msg);

    FileDownloader* fileDownloader = new FileDownloader();
    QDir baseDir(ConfigManager::instance()->dataSavePath());
    QString dir;

    QUrl url = QUrl(msg.file_urlValue());
    QString name = FileDownloader::extractBaseFileName(url);

    if(msg.isImage()){
        dir = baseDir.absoluteFilePath("images");
    }else if (msg.isVideo())
        dir = baseDir.absoluteFilePath("videos");
    else if(msg.isFile())
        dir = baseDir.absoluteFilePath("files");

    QString path = FileDownloader::generateUniqueFilePath(dir, name);

    fileDownloader->download(url, path);
    connect(fileDownloader, &FileDownloader::finished, this,
            [msg,this,fileDownloader,path](bool success, const QString& localPath){
                Orm orm;
                auto msgOpt = orm.findById<Message>(msg.message_idValue());
                if (msgOpt) {
                    msgOpt->setfile_path(localPath);
                    msgOpt->setfile_download_status(2);
                    orm.update(*msgOpt);
                    m_messagesModel->updateMessage(*msgOpt);
                }
                fileDownloader->deleteLater();
            });
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

void MessageController::handleZoom() { }
void MessageController::handleTranslate() { }
void MessageController::handleSearch() { }
void MessageController::handleForward() { }
void MessageController::handleFavorite() { }
void MessageController::handleRemind() { }
void MessageController::handleMultiSelect() { }
void MessageController::handleQuote() { }

