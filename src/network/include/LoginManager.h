#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebSocket>
#include <QSettings>

class LoginManager : public QObject
{
    Q_OBJECT
public:
    explicit LoginManager(QObject *parent = nullptr);
    ~LoginManager();

    void login(const QString &account, const QString &password);
    QString getToken() const { return m_token; }
    int64_t getUserId() const { return m_userId; }
    void connectWebSocket();
    void loadTokenFromStorage();
    void onWebSocketDisconnected();

signals:
    void loginSuccess();
    void loginFailed(const QString &reason);
    void networkError(const QString &error);
    void AutomaticLoginSuccessful(bool successful);

private slots:
    void onLoginReplyFinished();
    void onWebSocketConnected();
    void onWebSocketError(QAbstractSocket::SocketError error);

private:
    QNetworkAccessManager *m_networkManager;
    QWebSocket *m_webSocket;
    QString m_token;
    qint64 m_userId;
    QString m_nickname;
    QString m_avatar;
    int m_reconnectDelay = 5000; // 初始重连延迟5秒
    void saveTokenToStorage();
};

#endif // LOGINMANAGER_H
