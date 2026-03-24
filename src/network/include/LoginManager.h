#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtWebSockets/QWebSocket>
#include <QTimer>

class LoginManager : public QObject
{
    Q_OBJECT
public:
    explicit LoginManager(QObject *parent = nullptr);
    ~LoginManager() override;

    // 登录
    void login(const QString &account, const QString &password);
    // 注册（用户名方式）
    void registerUser(const QString &username, const QString &password, const QString &nickname = QString());
    // 注册（手机号方式）
    void registerUserByPhone(const QString &phone, const QString &password, const QString &nickname = QString());

    // 获取登录状态
    bool isLoggedIn() const { return !m_token.isEmpty(); }
    qint64 getUserId() const { return m_userId; }
    QString getNickname() const { return m_nickname; }
    QString getAvatar() const { return m_avatar; }
    QString getToken() const { return m_token; }

    // 自动登录（从存储加载 token）
    void attemptAutoLogin();

signals:
    // 登录相关信号
    void loginSuccess();
    void loginFailed(const QString &reason);
    void networkError(const QString &error);

    // 注册相关信号
    void registerSuccess(qint64 userId);
    void registerFailed(const QString &reason);

private slots:
    void onLoginReplyFinished();
    void onRegisterReplyFinished();
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(QAbstractSocket::SocketError error);

private:
    // 网络与 WebSocket
    QNetworkAccessManager *m_networkManager;
    QWebSocket *m_webSocket;
    QNetworkReply *m_currentRegisterReply = nullptr;  // 跟踪当前注册请求

    // 登录数据
    QString m_token;
    qint64 m_userId;
    QString m_nickname;
    QString m_avatar;

    // 重连机制
    int m_reconnectDelay;

    // 存储管理
    void saveTokenToStorage();
    void loadTokenFromStorage();
    void connectWebSocket();
    void clearLoginState();
};

#endif // LOGINMANAGER_H
