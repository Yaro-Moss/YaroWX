#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class LoginManager : public QObject
{
    Q_OBJECT
public:
    explicit LoginManager(QObject *parent = nullptr);
    ~LoginManager();

    void login(const QString &account, const QString &password);
    void registerUser(const QString &username, const QString &password, const QString &nickname = "");
    void registerUserByPhone(const QString &phone, const QString &password, const QString &nickname = "");

    // Token 管理（按用户ID独立存储）
    void saveTokenToStorage();
    void loadTokenFromStorage();
    void clearLoginState();

    // 获取用户信息
    bool isLoggedIn() const { return !m_token.isEmpty(); }
    QString getToken() const { return m_token; }
    qint64 getUserId() const { return m_userId; }
    QString getNickname() const { return m_nickname; }
    QString getAvatar() const { return m_avatar; }

signals:
    void loginSuccess();
    void loginFailed(const QString &error);
    void registerSuccess(qint64 userId);
    void registerFailed(const QString &error);
    void networkError(const QString &error);

private slots:
    void onLoginReplyFinished();
    void onRegisterReplyFinished();

private:
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentRegisterReply = nullptr;

    QString m_token;
    qint64 m_userId;
    QString m_nickname;
    QString m_avatar;
};

#endif // LOGINMANAGER_H