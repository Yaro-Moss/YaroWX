#include "WebSocketManager.h"
#include "ConfigManager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

WebSocketManager* WebSocketManager::m_instance = nullptr;
QMutex WebSocketManager::m_mutex;

WebSocketManager* WebSocketManager::instance()
{
    if (m_instance == nullptr) {
        QMutexLocker locker(&m_mutex);
        if (m_instance == nullptr) {
            m_instance = new WebSocketManager();
        }
    }
    return m_instance;
}

WebSocketManager::WebSocketManager(QObject* parent)
    : QObject(parent)
    , m_reconnectDelay(5000)
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);
    QObject::connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketManager::doReconnect);
}

WebSocketManager::~WebSocketManager()
{
    disconnect();
}

void WebSocketManager::setTokent(const QString& token)
{
    m_token = token;
}

void WebSocketManager::connect()
{
    if (m_webSocket) {
        if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
            qDebug() << "WebSocket already connected";
            return;
        }
        m_webSocket->disconnect();
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    m_webSocket = new QWebSocket();

    QObject::connect(m_webSocket, &QWebSocket::connected, this, &WebSocketManager::onConnected, Qt::UniqueConnection);
    QObject::connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketManager::onDisconnected, Qt::UniqueConnection);
    QObject::connect(m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketManager::onErrorOccurred, Qt::UniqueConnection);
    QObject::connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketManager::onTextMessageReceived, Qt::UniqueConnection);

    ConfigManager* config = ConfigManager::instance();
    QUrl url(config->webSocketUrl());
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());

    m_webSocket->open(request);
    qDebug() << "WebSocket connecting to" << url.toString();
}

void WebSocketManager::disconnect()
{
    if (m_reconnectTimer && m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }
    if (m_webSocket) {
        m_webSocket->close();
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }
    m_reconnectDelay = 5000;
}

bool WebSocketManager::isConnected() const
{
    return m_webSocket && (m_webSocket->state() == QAbstractSocket::ConnectedState);
}

void WebSocketManager::onConnected()
{
    qDebug() << "WebSocket connected";
    m_reconnectDelay = 5000;
    emit connected();
}

void WebSocketManager::onDisconnected()
{
    qDebug() << "WebSocket disconnected";
    emit disconnected();
    if (!m_token.isEmpty()) {
        scheduleReconnect();
    }
}

void WebSocketManager::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString errStr = m_webSocket->errorString();
    qDebug() << "WebSocket error:" << errStr;
    emit errorOccurred(errStr);
}

void WebSocketManager::sendTextMessage(const QString& message)
{
    if (!m_webSocket) {
        qWarning() << "发送失败：socket 未创建";
        emit messageSendFailed(-1, "网络连接有问题");
        // 主动触发重连
        if (!m_token.isEmpty() && !m_reconnectTimer->isActive()) {
            scheduleReconnect();
        }
        return;
    }
    if (m_webSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "发送失败：未连接，message:" << message.left(50);
        emit messageSendFailed(-1, "未连接");
        // 主动触发重连
        if (!m_token.isEmpty() && !m_reconnectTimer->isActive()) {
            scheduleReconnect();
        }
        return;
    }

    m_webSocket->sendTextMessage(message);
    qDebug() << "消息已发送：" << message.left(100);
}

void WebSocketManager::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "msg_ack") {
        qint64 clientTempId = 0;
        if (obj["client_temp_id"].isDouble()) {
            clientTempId = obj["client_temp_id"].toVariant().toLongLong();
        } else if (obj["client_temp_id"].isString()) {
            clientTempId = obj["client_temp_id"].toString().toLongLong();
        }
        int status = obj["status"].toInt();  // 0成功，非0失败
        if (status == 0) {
            emit messageAcknowledged(clientTempId, true);
        } else {
            QString error = QString("Send failed with status %1").arg(status);
            emit messageSendFailed(clientTempId, error);
        }
    }
    else if (type == "message_error") {
        qint64 localId = obj["local_message_id"].toVariant().toLongLong();
        QString error = obj["error"].toString();
        emit messageSendFailed(localId, error);
    }
    else if (type == "friend_request") {
        QJsonObject data = obj["data"].toObject();
        qint64 requestId = data["request_id"].toVariant().toLongLong();
        qint64 fromUserId = data["from_user_id"].toVariant().toLongLong();
        QString msg = data["message"].toString();
        emit friendRequestReceived(requestId, fromUserId, msg);
    }
    else if (type == "friend_request_accepted") {
        QJsonObject data = obj["data"].toObject();
        qint64 requestId = data["request_id"].toVariant().toLongLong();
        emit friendRequestProcessed(requestId, true);
    }
    else if (type == "friend_request_rejected") {
        QJsonObject data = obj["data"].toObject();
        qint64 requestId = data["request_id"].toVariant().toLongLong();
        emit friendRequestProcessed(requestId, false);
    }
    else {
        if (obj.contains("from_user_id") && obj.contains("chat_type") && obj.contains("content")) {
            emit newMessageReceived(obj);
        } else {
            qDebug() << "Received unknown message without type:" << message;
        }
    }
}

void WebSocketManager::scheduleReconnect()
{
    if (m_reconnectTimer->isActive()) return;

    qDebug() << "Scheduling WebSocket reconnect in" << m_reconnectDelay << "ms";
    m_reconnectTimer->start(m_reconnectDelay);
    m_reconnectDelay = qMin(m_reconnectDelay * 2, 60000);
}

void WebSocketManager::doReconnect()
{
    if (m_token.isEmpty()) {
        qDebug() << "No token, skip reconnect";
        return;
    }
    if (isConnected()) {
        qDebug() << "Already connected, skip reconnect";
        return;
    }
    qDebug() << "Attempting to reconnect...";
    connect();   // 使用保存的 token 重新连接
}