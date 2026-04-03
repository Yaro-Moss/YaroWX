#include "NetworkDataLoader.h"
#include "ConfigManager.h"
#include "FriendRequestItem.h"
#include "LoginManager.h"
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

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

bool NetworkDataLoader::searchUsers(const QString &keyword, QJsonArray &usersArray, QString &errorMessage, int page, int size)
{
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->getSearchUserUrl());

    // 构建查询参数
    QUrlQuery query;
    query.addQueryItem("keyword", keyword);
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("size", QString::number(size));
    url.setQuery(query);

    QJsonDocument doc;
    if (!sendGetRequest(url, doc, errorMessage)) {
        return false;
    }

    if (!doc.isArray()) {
        errorMessage = "返回数据不是数组";
        return false;
    }

    usersArray = doc.array();
    return true;
}


bool NetworkDataLoader::sendPostRequest(const QUrl &url, const QJsonObject &payload, QJsonDocument &responseDoc, QString &errorMessage, int timeoutMs)
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

    QByteArray postData = QJsonDocument(payload).toJson();
    QNetworkReply *reply = m_networkManager.post(request, postData);

    // 事件循环等待
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
        reply->abort();
        errorMessage = "请求超时";
    }

    reply->deleteLater();
    return success;
}



// ==============================================
// 1. 发送好友申请（带附加数据 + 无code判断）
// ==============================================
bool NetworkDataLoader::sendFriendRequest(
    qint64 to_user_id,
    const QString& message,
    const QString& remark,
    const QString& tags,
    const QString& source,
    const QString& description,
    qint64 &outRequestId,
    QString &errorMessage)
{
    errorMessage.clear();
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->getFriendRequestUrl());

    // 构建请求体
    QJsonObject payload;
    payload["to_user_id"] = to_user_id;
    payload["message"] = message;

    // 附加数据 applicant_meta（完整发送）
    QJsonObject applicantMeta;
    applicantMeta["remark"] = remark;
    applicantMeta["description"] = description;
    applicantMeta["source"] = source;
    applicantMeta["is_starred"] = 0;
    applicantMeta["is_blocked"] = 0;
    applicantMeta["phone_note"] = "";
    applicantMeta["email_note"] = "";

    // 标签字符串 → JSON数组
    QJsonArray tagArray;
    if (!tags.isEmpty()) {
        QStringList tagList = tags.split(",", Qt::SkipEmptyParts);
        for (const QString& tag : tagList) {
            tagArray.append(tag.trimmed());
        }
    }
    applicantMeta["tags"] = tagArray;
    payload["applicant_meta"] = applicantMeta;

    // 发送请求
    QJsonDocument responseDoc;
    if (!sendPostRequest(url, payload, responseDoc, errorMessage)) {
        return false;
    }

    // 解析响应
    if (!responseDoc.isObject()) {
        errorMessage = "响应格式错误";
        return false;
    }
    QJsonObject resObj = responseDoc.object();

    // ===================== 后端已删除code，直接判断error =====================
    if (resObj.contains("error")) {
        errorMessage = resObj["error"].toString();
        return false;
    }

    // 成功：直接获取申请ID
    outRequestId = resObj["request_id"].toVariant().toLongLong();
    return true;
}

// ==============================================
// 2. 获取待处理好友申请列表
// ==============================================
bool NetworkDataLoader::getPendingFriendRequests(
    QList<FriendRequestItem> &outList,
    QString &errorMessage)
{
    errorMessage.clear();
    outList.clear();
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->getPendingRequestsUrl());

    QJsonDocument responseDoc;
    if (!sendGetRequest(url, responseDoc, errorMessage)) {
        return false;
    }

    // 错误判断
    if (responseDoc.isObject()) {
        QJsonObject obj = responseDoc.object();
        if (obj.contains("error")) {
            errorMessage = obj["error"].toString();
            return false;
        }
    }

    // 成功：解析数组
    if (!responseDoc.isArray()) {
        errorMessage = "响应格式错误";
        return false;
    }

    QJsonArray array = responseDoc.array();
    for (const QJsonValue& val : array) {
        QJsonObject obj = val.toObject();
        FriendRequestItem item;

        item.id = obj["id"].toVariant().toLongLong();
        item.fromUserId = obj["from_user_id"].toVariant().toLongLong();
        item.message = obj["message"].toString();
        item.status = obj["status"].toInt();
        item.createdAt = obj["created_at"].toString();
        item.fromNickname = obj["from_nickname"].toString();
        item.fromAvatar = obj["from_avatar"].toString();
        item.fromAccount = obj["from_account"].toString();

        outList.append(item);
    }

    return true;
}

// ==============================================
// 3. 处理好友申请（同意=1 / 拒绝=2）
// ==============================================
bool NetworkDataLoader::processFriendRequest(
    qint64 requestId,
    bool agree,  // true=同意  false=拒绝
    QString &errorMessage)
{
    errorMessage.clear();
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->processRequestUrl().arg(requestId));

    QJsonObject payload;
    payload["status"] = agree ? 1 : 2;

    QJsonDocument responseDoc;
    if (!sendPostRequest(url, payload, responseDoc, errorMessage)) {
        return false;
    }

    if (!responseDoc.isObject()) {
        errorMessage = "响应格式错误";
        return false;
    }
    QJsonObject resObj = responseDoc.object();

    // 仅判断错误
    if (resObj.contains("error")) {
        errorMessage = resObj["error"].toString();
        return false;
    }

    // 无code，直接成功
    return true;
}


