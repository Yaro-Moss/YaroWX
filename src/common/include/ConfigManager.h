#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>

class ConfigManager : public QObject
{
    Q_OBJECT
    // 单例禁止拷贝和赋值，符合设计规范
    Q_DISABLE_COPY(ConfigManager)

private:
    QSettings* m_settings;
    QString currentUserID = "";

    explicit ConfigManager(QObject *parent = nullptr);

    // 初始化配置
    void initConfig();

    void createDefaultConfig(const QString& configPath);

public:
    static ConfigManager* instance();

    // 配置读取接口
    QString webSocketUrl() const;
    QString loginUrl() const;
    int maxRetry() const;
    QString dataSavePath() const;

    // 保存配置
    void saveConfig();

    ~ConfigManager() override;
    void setCurrentUserID(QString id){currentUserID = id;}

};

#endif // CONFIGMANAGER_H
