#include "NetworkDataLoader.h"
#include "ConfigManager.h"
#include "FriendRequest.h"
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

bool NetworkDataLoader::sendGetRequest(const QUrl &url,
                                       QJsonDocument &responseDoc,
                                       QString &errorMessage,
                                       int timeoutMs)
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

    // 改为局部 QNetworkAccessManager（无父对象，线程安全）
    QNetworkAccessManager localManager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = localManager.get(request);

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

bool NetworkDataLoader::sendPostRequest(const QUrl &url,
                                        const QJsonObject &payload,
                                        QJsonDocument &responseDoc,
                                        QString &errorMessage,
                                        int timeoutMs)
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

    QNetworkAccessManager localManager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QByteArray postData = QJsonDocument(payload).toJson();
    QNetworkReply *reply = localManager.post(request, postData);

    // 2. 事件循环等待（保持不变）
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

bool NetworkDataLoader::sendPutRequest(const QUrl &url,
                                       const QJsonObject &payload,
                                       QJsonDocument &responseDoc,
                                       QString &errorMessage,
                                       int timeoutMs)
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

    // 局部 QNetworkAccessManager，避免跨线程问题
    QNetworkAccessManager localManager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QByteArray putData = QJsonDocument(payload).toJson();
    QNetworkReply *reply = localManager.put(request, putData);

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

bool NetworkDataLoader::sendDeleteRequest(const QUrl &url,
                                          QJsonDocument &responseDoc,
                                          QString &errorMessage,
                                          int timeoutMs)
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

    QNetworkAccessManager localManager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = localManager.deleteResource(request);  // DELETE 请求

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
        for (const QString& tag : std::as_const(tagList)) {
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
    QList<FriendRequest>& outList,
    QString& errorMessage)
{
    errorMessage.clear();
    outList.clear();
    ConfigManager* config = ConfigManager::instance();
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
    for (const QJsonValue& val : std::as_const(array)) {
        QJsonObject obj = val.toObject();
        FriendRequest request;

        // 使用专用 setter 设置服务端字段
        request.setid(obj["id"].toVariant().toLongLong());
        request.setfrom_user_id(obj["from_user_id"].toVariant().toLongLong());
        request.setmessage(obj["message"].toString());
        request.setstatus(obj["status"].toInt());

        // 时间转换：服务端返回 "yyyy-MM-dd HH:mm:ss" 本地时间 → Unix 时间戳
        QString createdAtStr = obj["created_at"].toString();
        qint64 timestamp = 0;
        if (!createdAtStr.isEmpty()) {
            QDateTime dt = QDateTime::fromString(createdAtStr, "yyyy-MM-dd HH:mm:ss");
            if (dt.isValid()) {
                timestamp = dt.toSecsSinceEpoch();
            }
        }
        request.setcreated_at(timestamp);

        request.setfrom_nickname(obj["from_nickname"].toString());
        request.setfrom_avatar(obj["from_avatar"].toString());
        request.setfrom_account(obj["from_account"].toString());

        // 本地扩展字段默认值
        request.setis_read(0);
        request.setlocal_processed(0);

        outList.append(request);
    }

    return true;
}

// ==============================================
// 3. 处理好友申请
// ==============================================
bool NetworkDataLoader::processFriendRequest(
    qint64 requestId,
    bool agree,
    QString& errorMessage,
    const QJsonObject& meta)
{
    errorMessage.clear();
    ConfigManager *config = ConfigManager::instance();
    QUrl url(config->processRequestUrl().arg(requestId));

    QJsonObject payload;
    payload["status"] = agree ? 1 : 2;
    if (!meta.isEmpty()) {
        payload["meta"] = meta;   // 附加被申请人的备注、标签等信息
    }

    QJsonDocument responseDoc;
    // 关键修改：改为 sendPutRequest
    if (!sendPutRequest(url, payload, responseDoc, errorMessage)) {
        return false;
    }

    if (!responseDoc.isObject()) {
        errorMessage = "响应格式错误";
        return false;
    }
    QJsonObject resObj = responseDoc.object();

    if (resObj.contains("error")) {
        errorMessage = resObj["error"].toString();
        return false;
    }
    return true;
}


bool NetworkDataLoader::getFriend(qint64 friendId,
                                  QJsonObject &friendData,
                                  QString &errorMessage)
{
    errorMessage.clear();
    ConfigManager *config = ConfigManager::instance();
    QString urlStr = config->getFriendURL().arg( QString::number(friendId));
    QUrl url(urlStr);

    QJsonDocument responseDoc;
    if (!sendGetRequest(url, responseDoc, errorMessage)) {
        return false;
    }

    if (!responseDoc.isObject()) {
        errorMessage = "响应格式错误，期望 JSON 对象";
        return false;
    }

    QJsonObject obj = responseDoc.object();
    if (obj.contains("error")) {
        errorMessage = obj["error"].toString();
        return false;
    }

    friendData = obj;
    return true;
}


bool NetworkDataLoader::updateFriend(qint64 friendId,
                                     const QJsonObject &updateData,
                                     QString &errorMessage)
{
    errorMessage.clear();
    ConfigManager *config = ConfigManager::instance();
    QString urlStr = config->deleteFriendURL().arg(QString::number(friendId));
    QUrl url(urlStr);

    // 过滤掉服务端不允许的字段（客户端调用时最好只传允许的字段，这里再做一次保险）
    QJsonObject safeData = updateData;
    static const QStringList forbidden = {"id", "user_id", "friend_id", "created_at", "updated_at", "status"};
    for (const QString &key : forbidden) {
        safeData.remove(key);
    }

    QJsonDocument responseDoc;
    if (!sendPutRequest(url, safeData, responseDoc, errorMessage)) {
        return false;
    }

    if (!responseDoc.isObject()) {
        errorMessage = "响应格式错误";
        return false;
    }

    QJsonObject obj = responseDoc.object();
    if (obj.contains("error")) {
        errorMessage = obj["error"].toString();
        return false;
    }

    // 成功时服务端返回 {"success": true}，也可以不检查具体内容
    return true;
}


bool NetworkDataLoader::deleteFriend(qint64 friendId, QString &errorMessage)
{
    errorMessage.clear();
    ConfigManager *config = ConfigManager::instance();
    QString urlStr = config->updateFriendURL().arg(QString::number(friendId));
    QUrl url(urlStr);

    QJsonDocument responseDoc;  // 204 响应可能为空，但 sendDeleteRequest 仍会尝试解析 JSON
    if (!sendDeleteRequest(url, responseDoc, errorMessage)) {
        // 如果是因为解析空响应失败，特殊处理
        if (errorMessage.contains("JSON 解析失败") && responseDoc.isNull()) {
            errorMessage.clear();
            return true;  // 204 无内容视为成功
        }
        return false;
    }

    // 如果有响应体，检查是否有错误字段
    if (!responseDoc.isNull() && responseDoc.isObject()) {
        QJsonObject obj = responseDoc.object();
        if (obj.contains("error")) {
            errorMessage = obj["error"].toString();
            return false;
        }
    }
    return true;
}






