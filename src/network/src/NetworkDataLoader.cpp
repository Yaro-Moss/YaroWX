#include "NetworkDataLoader.h"
#include "ConfigManager.h"
#include "LoginManager.h"
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QJsonParseError>

NetworkDataLoader::NetworkDataLoader(LoginManager *loginManager, QObject *parent)
    : QObject(parent), m_loginManager(loginManager)
{
}

bool NetworkDataLoader::sendGetRequest(const QUrl &url, QJsonDocument &responseDoc, QString &errorMessage, int timeoutMs)
{
    if (!m_loginManager->isLoggedIn()) {
        errorMessage = "用户未登录";
        return false;
    }

    QString token = m_loginManager->getToken();
    if (token.isEmpty()) {
        errorMessage = "Token 为空";
        return false;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = m_networkManager.get(request);

    // 使用事件循环阻塞等待
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeoutTimer.start(timeoutMs);
    loop.exec();

    bool success = false;
    if (timeoutTimer.isActive()) {
        timeoutTimer.stop();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonParseError parseError;
            responseDoc = QJsonDocument::fromJson(data, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                success = true;
            } else {
                errorMessage = "JSON 解析失败: " + parseError.errorString();
            }
        } else {
            errorMessage = reply->errorString();
        }
    } else {
        // 超时
        reply->abort();
        errorMessage = "请求超时";
    }

    reply->deleteLater();
    return success;
}

bool NetworkDataLoader::loadUserInfo(QJsonObject &userInfo, QString &errorMessage)
{
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->getProfileUrl());
    QJsonDocument doc;
    if (!sendGetRequest(url, doc, errorMessage)) {
        return false;
    }
    if (!doc.isObject()) {
        errorMessage = "返回数据格式错误";
        return false;
    }
    userInfo = doc.object();
    return true;
}

bool NetworkDataLoader::loadFriends(QJsonArray &friendsArray, QString &errorMessage)
{
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->getAllFriendUrl());
    // 可添加排序、分页参数，这里先不加
    QJsonDocument doc;
    if (!sendGetRequest(url, doc, errorMessage)) {
        return false;
    }
    if (!doc.isArray()) {
        errorMessage = "好友数据不是数组";
        return false;
    }
    friendsArray = doc.array();
    return true;
}

bool NetworkDataLoader::loadGroupsAddMembers(QJsonArray &groupsArray, QString &errorMessage)
{
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->getGroupsAddMembersUrl());
    QJsonDocument doc;
    if (!sendGetRequest(url, doc, errorMessage)) {
        return false;
    }
    if (!doc.isArray()) {
        errorMessage = "群组数据不是数组";
        return false;
    }
    groupsArray = doc.array();
    return true;
}
