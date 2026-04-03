#ifndef NETWORKDATALOADER_H
#define NETWORKDATALOADER_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

class LoginManager; // 前向声明
struct  FriendRequestItem;

class NetworkDataLoader : public QObject
{
    Q_OBJECT
public:
    explicit NetworkDataLoader(LoginManager *loginManager, QObject *parent = nullptr);

    // 同步加载用户信息（当前登录用户）
    bool loadUserInfo(QJsonObject &userInfo, QString &errorMessage);

    // 同步加载好友列表（包含好友的公开信息）
    bool loadFriends(QJsonArray &friendsArray, QString &errorMessage);

    // 同步加载群组列表（用户加入的群组）
    bool loadGroupsAddMembers(QJsonArray &groupsArray, QString &errorMessage);

    // 搜索用户（支持分页）
    bool searchUsers(const QString &keyword, QJsonArray &usersArray, QString &errorMessage, int page = 1, int size = 20);

    // 发送好友申请
    bool sendFriendRequest(qint64 to_user_id,
                           const QString& message,
                           const QString& remark,
                           const QString& tags,
                           const QString& source,
                           const QString& description,
                           qint64 &outRequestId,
                           QString &errorMessage);

    bool getPendingFriendRequests(QList<FriendRequestItem> &outList,
                                  QString &errorMessage);

    bool processFriendRequest(qint64 requestId, bool agree, QString &errorMessage);

private:
    // 通用同步请求方法
    bool sendGetRequest(const QUrl &url, QJsonDocument &responseDoc, QString &errorMessage, int timeoutMs = 10000);

    bool sendPostRequest(const QUrl &url, const QJsonObject &payload, QJsonDocument &responseDoc, QString &errorMessage, int timeoutMs = 10000);

    LoginManager *m_loginManager;
    QNetworkAccessManager m_networkManager;
};

#endif // NETWORKDATALOADER_H
