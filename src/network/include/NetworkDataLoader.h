#ifndef NETWORKDATALOADER_H
#define NETWORKDATALOADER_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

class LoginManager; // 前向声明

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

private:
    // 通用同步请求方法
    bool sendGetRequest(const QUrl &url, QJsonDocument &responseDoc, QString &errorMessage, int timeoutMs = 10000);

    LoginManager *m_loginManager;
    QNetworkAccessManager m_networkManager;
};

#endif // NETWORKDATALOADER_H
