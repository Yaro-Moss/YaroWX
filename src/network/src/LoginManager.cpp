#include "LoginManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <qthread.h>
#include <qtimer.h>
#include "ConfigManager.h"

LoginManager::LoginManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_webSocket(nullptr)
    , m_token("")
    , m_userId(0)
{
    // 启动时从存储加载上次的 token（用于自动登录）
    // loadTokenFromStorage();
}

LoginManager::~LoginManager()
{
    qDebug() << "LoginManager 析构，this =" << this << QThread::currentThread();
    if (m_webSocket) {
        m_webSocket->close();
        m_webSocket->deleteLater();
    }
}

void LoginManager::login(const QString &account, const QString &password)
{
    // 构建请求 URL
    ConfigManager* configManager = ConfigManager::instance();
    QUrl url(configManager->loginUrl());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 构建 JSON 数据
    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // 发送 POST 请求
    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &LoginManager::onLoginReplyFinished);
}

void LoginManager::onLoginReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        // 网络错误（如连接失败、超时）
        emit networkError(reply->errorString());
        reply->deleteLater();
        return;
    }

    // 检查 HTTP 状态码
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();

    if (statusCode == 200) {
        // 登录成功
        m_token = obj["token"].toString();
        m_userId = obj["user_id"].toInteger();
        m_nickname = obj["nickname"].toString();
        m_avatar = obj["avatar"].toString();
        ConfigManager* configManager = ConfigManager::instance();
        configManager->setCurrentUserID( QString::number(m_userId));

        qDebug() << "登陆成功" << m_userId;
        qDebug() << "nickname" << m_nickname;

        saveTokenToStorage();      // 存储 token
        connectWebSocket();        // 连接 WebSocket

    } else {
        // 登录失败，从 JSON 中提取错误信息
        QString errorMsg = obj["error"].toString("登录失败");
        emit loginFailed(errorMsg);
    }
}

void LoginManager::saveTokenToStorage()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("token", m_token);
    settings.setValue("userId", m_userId);
    settings.setValue("nickname", m_nickname);
    settings.setValue("avatar", m_avatar);
}

void LoginManager::loadTokenFromStorage()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    m_token = settings.value("token").toString();
    m_userId = settings.value("userId").toLongLong();
    m_nickname = settings.value("nickname").toString();
    m_avatar = settings.value("avatar").toString();
    qDebug() << "配置文件路径：" << settings.fileName();

    if (!m_token.isEmpty()) {
        // 直接连接 WebSocket（假设 token 仍然有效）
        connectWebSocket();
    }
}

void LoginManager::connectWebSocket()
{
    if (m_webSocket) {
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }
    m_webSocket = new QWebSocket();

    // 连接信号
    connect(m_webSocket, &QWebSocket::connected, this, &LoginManager::onWebSocketConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &LoginManager::onWebSocketDisconnected);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &LoginManager::onWebSocketError);
    connect(m_webSocket, &QWebSocket::textMessageReceived, [](const QString &message) {
        qDebug() << "Received message:" << message;
        // 处理收到的消息
    });

    // 设置请求头
    ConfigManager* configManager = ConfigManager::instance();
    QUrl url(configManager->webSocketUrl());
    QNetworkRequest request(url);
    QString authHeader = "Bearer " + m_token;
    request.setRawHeader("Authorization", authHeader.toUtf8());

    m_webSocket->open(request);
}

void LoginManager::onWebSocketConnected()
{
    qDebug() << "WebSocket connected";

    // 登录成功信号
    emit AutomaticLoginSuccessful(true);
    emit loginSuccess();
}

void LoginManager::onWebSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << "WebSocket error:" << m_webSocket->errorString();
    emit networkError(m_webSocket->errorString());
    emit AutomaticLoginSuccessful(false);
}

// 重连机制
void LoginManager::onWebSocketDisconnected()
{
    qDebug() << "WebSocket disconnected. Attempting to reconnect...";

    // 指数退避：第一次等待5秒，第二次10秒，第三次20秒...
    m_reconnectDelay = qMin(m_reconnectDelay * 2, 60000); // 最大等待1分钟
    QTimer::singleShot(m_reconnectDelay, this, &LoginManager::connectWebSocket);
}
