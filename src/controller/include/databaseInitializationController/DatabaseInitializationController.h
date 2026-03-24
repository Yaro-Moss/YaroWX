#ifndef DATABASEINITIALIZATIONCONTROLLER_H
#define DATABASEINITIALIZATIONCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>

class DatabaseInitializer;
class NetworkDataLoader;
class LoginManager;

class DatabaseInitializationController : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseInitializationController(LoginManager *loginManager, QObject *parent = nullptr);
    ~DatabaseInitializationController();

    void initialize();

signals:
    void initializationProgress(int progress, const QString& message);
    void initializationFinished(bool success, const QString& errorMessage = "");
    void databaseReady();

private:
    void updateProgress(int progress, const QString& message);
    bool stepLoadUserInfo();      // 步骤1
    bool stepLoadFriends();       // 步骤2
    bool stepLoadGroupsAddMembers();        // 步骤3
    void completeInitialization(bool success, const QString &errorMsg = "");

    // 数据存储（这里需要根据您的 DatabaseInitializer 接口调整）
    bool saveUserInfoToDb(const QJsonObject &userInfo);
    bool saveFriendsToDb(const QJsonArray &friendsArray);
    bool saveGroupsAddMembersToDb(const QJsonArray &groupsArray);

private:
    bool m_initializationInProgress;
    int m_currentProgress;
    DatabaseInitializer *databaseInitializer;
    NetworkDataLoader *m_networkLoader;
    LoginManager *m_loginManager;

    // 临时保存网络数据
    QJsonObject m_userInfo;
    QJsonArray m_friendsArray;
    QJsonArray m_groupsArray;
    QJsonArray m_membersArray;
    qint64 m_currentUserId;
};

#endif // DATABASEINITIALIZATIONCONTROLLER_H
