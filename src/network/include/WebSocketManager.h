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

    void connect();
    void setTokent(const QString& token);
    void disconnect();
    bool isConnected() const;
    void sendTextMessage(const QString& message);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

    void friendRequestReceived(qint64 requestId, qint64 fromUserId, const QString& message);
    void friendRequestProcessed(qint64 requestId, bool accepted);

    void messageAcknowledged(qint64 localId, bool success);
    void messageSendFailed(qint64 localId, const QString& error);
    void newMessageReceived(const QJsonObject& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString& message);
    void scheduleReconnect();
    void doReconnect();

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