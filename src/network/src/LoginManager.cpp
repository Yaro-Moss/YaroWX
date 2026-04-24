#include "LoginManager.h"
#include "ConfigManager.h"
#include "WebSocketManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QtWidgets/QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

LoginManager::LoginManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_token("")
    , m_userId(0)
{
}

LoginManager::~LoginManager()
{
    // WebSocketManager 是单例，无需在此清理
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

        emit loginSuccess();       // 通知登录成功
        saveTokenToStorage();      // 存储 token
        WebSocketManager::instance()->setTokent(m_token);

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

// ====================== 按用户ID保存Token ======================
void LoginManager::saveTokenToStorage()
{
    if (m_userId <= 0) {
        qDebug() << "保存失败：用户ID无效";
        return;
    }

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(configPath);
    dir.mkdir("users");
    QString userConfigFile = dir.filePath(QString("users/user_%1.ini").arg(m_userId));

    QSettings settings(userConfigFile, QSettings::IniFormat);
    settings.setValue("token", m_token);
    settings.setValue("userId", m_userId);
    settings.setValue("nickname", m_nickname);
    settings.setValue("avatar", m_avatar);
    qDebug() << "保存当前用户登录信息到：" << settings.fileName();
}

void LoginManager::loadTokenFromStorage()
{
    if (m_userId <= 0) {
        qDebug() << "读取失败：用户ID无效";
        return;
    }

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(configPath);
    QString userConfigFile = dir.filePath(QString("users/user_%1.ini").arg(m_userId));

    QSettings settings(userConfigFile, QSettings::IniFormat);
    m_token = settings.value("token").toString();
    m_userId = settings.value("userId").toLongLong();
    m_nickname = settings.value("nickname").toString();
    m_avatar = settings.value("avatar").toString();
    qDebug() << "加载当前用户登录信息从：" << settings.fileName();
}

void LoginManager::clearLoginState()
{
    // 先清空本地变量
    m_token.clear();
    m_userId = 0;
    m_nickname.clear();
    m_avatar.clear();

    // 断开 WebSocket
    WebSocketManager::instance()->disconnect();

    // 删除当前用户的配置文件
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(configPath);
    QString userConfigFile = dir.filePath(QString("users/user_%1.ini").arg(m_userId));
    QSettings settings(userConfigFile, QSettings::IniFormat);
    settings.clear();
    qDebug() << "已清除当前用户的登录信息";
}