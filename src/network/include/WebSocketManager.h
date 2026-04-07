#ifndef WEBSOCKETMANAGER_H
#define WEBSOCKETMANAGER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QTimer>
#include <QJsonObject>
#include <QMutex>

class WebSocketManager : public QObject
{
    Q_OBJECT

public:
    static WebSocketManager* instance();

    void connect(const QString& token);
    void disconnect();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& errorString);

    // 好友申请相关信号
    void friendRequestReceived(qint64 requestId, qint64 fromUserId, const QString& message);
    void friendRequestProcessed(qint64 requestId, bool accepted); // true=同意, false=拒绝

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString& message);
    void scheduleReconnect();

private:
    explicit WebSocketManager(QObject* parent = nullptr);
    ~WebSocketManager();

    QWebSocket* m_webSocket = nullptr;
    QString m_token;
    int m_reconnectDelay;
    QTimer* m_reconnectTimer = nullptr;

    static WebSocketManager* m_instance;
    static QMutex m_mutex;
};

#endif // WEBSOCKETMANAGER_H