#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>

class ConfigManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ConfigManager)

private:
    QSettings* m_settings;
    QString currentLoginUserID = "";

    explicit ConfigManager(QObject *parent = nullptr);
    void initConfig();
    void createDefaultConfig(const QString& configPath);

public:
    static ConfigManager* instance();

    // 基础地址
    QString baseUrl() const;

    // WebSocket 地址（不参与 baseUrl 拼接）
    QString webSocketUrl() const;

    // HTTP API 地址（基于 baseUrl 拼接）
    QString loginUrl() const;
    QString registerUrl() const;
    QString getAllFriendUrl() const;
    QString getProfileUrl() const;
    QString getGroupsAddMembersUrl() const;
    QString getSearchUserUrl() const;
    QString getFriendRequestUrl() const;
    QString getPendingRequestsUrl() const;
    QString processRequestUrl() const;   // 带 %1 占位符
    QString getFriendURL() const;        // 带 %1 占位符
    QString updateFriendURL() const;     // 带 %1 占位符
    QString deleteFriendURL() const;     // 带 %1 占位符
    QString uploadUrl() const;           // 可选，原配置中有 UploadUrl
    QString getUploadUrl() const;        // 获取上传文件 URL

    // 其他配置
    int maxRetry() const;
    QString dataSavePath() const;

    void saveConfig();
    void setCurrentLoginUserID(QString id) { currentLoginUserID = id; }

    ~ConfigManager() override;
};

#endif // CONFIGMANAGER_H