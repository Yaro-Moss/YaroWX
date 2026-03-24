#include "LoginManager.h"
#include "ConfigManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QDebug>

LoginManager::LoginManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_webSocket(nullptr)
    , m_token("")
    , m_userId(0)
    , m_reconnectDelay(5000)   // 初始重连延迟 5 秒
{
}

LoginManager::~LoginManager()
{
    if (m_webSocket) {
        m_webSocket->close();
        m_webSocket->deleteLater();
    }
}

// -------------------- 登录 --------------------
void LoginManager::login(const QString &account, const QString &password)
{
    if (account.isEmpty() || password.isEmpty()) {
        emit loginFailed("账号或密码不能为空");
        return;
    }

    ConfigManager *configManager = ConfigManager::instance();
    QUrl url(configManager->loginUrl());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &LoginManager::onLoginReplyFinished);
}

void LoginManager::onLoginReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        emit networkError(reply->errorString());
        reply->deleteLater();
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();

    if (statusCode == 200) {
        // 登录成功，保存登录信息
        m_token = obj["token"].toString();
        m_userId = obj["user_id"].toInteger();
        m_nickname = obj["nickname"].toString();
        m_avatar = obj["avatar"].toString();

        ConfigManager *configManager = ConfigManager::instance();
        configManager->setCurrentLoginUserID(QString::number(m_userId));

        qDebug() << "登录成功" << m_userId;
        qDebug() << "nickname" << m_nickname;

        emit loginSuccess();       // 通知登录成功
        saveTokenToStorage();      // 存储 token
        connectWebSocket();        // 连接 WebSocket

    } else {
        QString errorMsg = obj["error"].toString("登录失败");
        emit loginFailed(errorMsg);
    }
}

// -------------------- 注册 --------------------
void LoginManager::registerUser(const QString &username, const QString &password, const QString &nickname)
{
    if (username.isEmpty() || password.isEmpty()) {
        emit registerFailed("用户名或密码不能为空");
        return;
    }

    ConfigManager *configManager = ConfigManager::instance();
    QUrl url(configManager->registerUrl());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    if (!nickname.isEmpty())
        json["nickname"] = nickname;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    if (m_currentRegisterReply) {
        m_currentRegisterReply->abort();
        m_currentRegisterReply->deleteLater();
    }

    m_currentRegisterReply = m_networkManager->post(request, data);
    connect(m_currentRegisterReply, &QNetworkReply::finished, this, &LoginManager::onRegisterReplyFinished);
}

void LoginManager::registerUserByPhone(const QString &phone, const QString &password, const QString &nickname)
{
    if (phone.isEmpty() || password.isEmpty()) {
        emit registerFailed("手机号或密码不能为空");
        return;
    }

    ConfigManager *configManager = ConfigManager::instance();
    QUrl url(configManager->registerUrl());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["phone"] = phone;
    json["password"] = password;
    if (!nickname.isEmpty())
        json["nickname"] = nickname;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    if (m_currentRegisterReply) {
        m_currentRegisterReply->abort();
        m_currentRegisterReply->deleteLater();
    }

    m_currentRegisterReply = m_networkManager->post(request, data);
    connect(m_currentRegisterReply, &QNetworkReply::finished, this, &LoginManager::onRegisterReplyFinished);
}

void LoginManager::onRegisterReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    m_currentRegisterReply = nullptr;   // 清空指针

    if (reply->error() != QNetworkReply::NoError) {
        emit registerFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();

    if (statusCode == 201 || statusCode == 200) {
        qint64 userId = obj["user_id"].toInteger();
        emit registerSuccess(userId);
    } else {
        QString errorMsg = obj["error"].toString("注册失败");
        emit registerFailed(errorMsg);
    }
}

// -------------------- token存储 --------------------
void LoginManager::saveTokenToStorage()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("token", m_token);
    settings.setValue("userId", m_userId);
    settings.setValue("nickname", m_nickname);
    settings.setValue("avatar", m_avatar);
    qDebug() << "保存登录信息到：" << settings.fileName();
}

void LoginManager::loadTokenFromStorage()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    m_token = settings.value("token").toString();
    m_userId = settings.value("userId").toLongLong();
    m_nickname = settings.value("nickname").toString();
    m_avatar = settings.value("avatar").toString();
    qDebug() << "加载登录信息从：" << settings.fileName();
}

void LoginManager::clearLoginState()
{
    m_token.clear();
    m_userId = 0;
    m_nickname.clear();
    m_avatar.clear();
    if (m_webSocket) {
        m_webSocket->close();
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.remove("token");
    settings.remove("userId");
    settings.remove("nickname");
    settings.remove("avatar");
}

// -------------------- WebSocket --------------------
void LoginManager::connectWebSocket()
{
    if (m_webSocket) {
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    m_webSocket = new QWebSocket();

    connect(m_webSocket, &QWebSocket::connected, this, &LoginManager::onWebSocketConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &LoginManager::onWebSocketDisconnected);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &LoginManager::onWebSocketError);
    connect(m_webSocket, &QWebSocket::textMessageReceived, [](const QString &message) {
        qDebug() << "Received message:" << message;
        // 此处可转发给其他模块处理
    });

    ConfigManager *configManager = ConfigManager::instance();
    QUrl url(configManager->webSocketUrl());
    QNetworkRequest request(url);
    QString authHeader = "Bearer " + m_token;
    request.setRawHeader("Authorization", authHeader.toUtf8());

    m_webSocket->open(request);
}

void LoginManager::onWebSocketConnected()
{
    qDebug() << "WebSocket connected";
    // 重连延迟重置
    m_reconnectDelay = 5000;
}

void LoginManager::onWebSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qDebug() << "WebSocket error:" << m_webSocket->errorString();
    QMessageBox::critical(nullptr, "网络错误", "网络连接失败: "+m_webSocket->errorString());
}

void LoginManager::onWebSocketDisconnected()
{
    qDebug() << "WebSocket disconnected. Attempting to reconnect...";
    // 指数退避：第一次等待5秒，第二次10秒，第三次20秒... 最大1分钟
    m_reconnectDelay = qMin(m_reconnectDelay * 2, 60000);
    QTimer::singleShot(m_reconnectDelay, this, &LoginManager::connectWebSocket);
}
