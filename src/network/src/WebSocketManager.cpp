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
    QObject::connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketManager::scheduleReconnect);
}

WebSocketManager::~WebSocketManager()
{
    disconnect();
}

void WebSocketManager::connect(const QString& token)
{
    if (m_webSocket) {
        if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
            qDebug() << "WebSocket already connected";
            return;
        }
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    m_token = token;
    m_webSocket = new QWebSocket();

    QObject::connect(m_webSocket, &QWebSocket::connected, this, &WebSocketManager::onConnected);
    QObject::connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketManager::onDisconnected);
    QObject::connect(m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketManager::onErrorOccurred);
    QObject::connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketManager::onTextMessageReceived);

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

void WebSocketManager::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qWarning() << "Invalid WebSocket message (not JSON object):" << message;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "friend_request") {
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
        qDebug() << "Unhandled WebSocket message type:" << type;
    }
}

void WebSocketManager::scheduleReconnect()
{
    if (m_reconnectTimer->isActive()) return;

    qDebug() << "Scheduling WebSocket reconnect in" << m_reconnectDelay << "ms";
    m_reconnectTimer->start(m_reconnectDelay);
    m_reconnectDelay = qMin(m_reconnectDelay * 2, 60000);
}