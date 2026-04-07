#include "ConfigManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    , m_settings(nullptr)
{
    initConfig();
}

ConfigManager::~ConfigManager()
{
    if (m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
}

ConfigManager* ConfigManager::instance()
{
    static ConfigManager instance;
    return &instance;
}

void ConfigManager::initConfig()
{
    // 获取应用配置目录 (跨平台)
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);

    // 配置文件路径
    QString configPath = configDir + "/config.ini";

    if (!QFile::exists(configPath)) {
        createDefaultConfig(configPath);
    }

    if (m_settings) {
        delete m_settings;
    }
    m_settings = new QSettings(configPath, QSettings::IniFormat);
}

void ConfigManager::createDefaultConfig(const QString& configPath)
{
    QFile configFile(configPath);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);

        out << "# " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
        out << "[General]\n";
        out << "AppName=YaroWX\n";
        out << "Version=1.0.0\n\n";

        out << "[Network]\n";
        out << "WebSocketUrl=ws://localhost:8080/chat\n";
        out << "LoginUrl=http://localhost:8080/api/v1/login\n";
        out << "MaxRetry=5\n\n";

        out << "[Data]\n";
        out << "SavePath=" << QDir::homePath() + "/YaroWX/data" << "\n";

        configFile.close();
    }
}

QString ConfigManager::webSocketUrl() const
{
    if (!m_settings) return "ws://localhost:8080/chat";
    return m_settings->value("Network/WebSocketUrl", "ws://localhost:8080/chat").toString();
}

QString ConfigManager::loginUrl() const
{
    if (!m_settings) return "http://localhost:8080/api/v1/login";
    return m_settings->value("Network/LoginUrl", "http://localhost:8080/api/v1/login").toString();
}

QString ConfigManager::registerUrl() const
{
    if (!m_settings) return "http://localhost:8080/api/v1/register";
    return m_settings->value("Network/RegisterUrl", "http://localhost:8080/api/v1/register").toString();
}

QString ConfigManager::getAllFriendUrl() const
{
    if (!m_settings) return "http://localhost:8080/api/v1/friend";
    return m_settings->value("Network/GetAllFriendUrl", "http://localhost:8080/api/v1/friend").toString();
}

QString ConfigManager::getProfileUrl() const
{
    if (!m_settings) return "http://localhost:8080/api/user/profile";
    return m_settings->value("Network/GetProfileUrl", "http://localhost:8080/api/user/profile").toString();
}

QString ConfigManager::getGroupsAddMembersUrl() const
{
    if (!m_settings) return "http://localhost:8080/api/user/groupsAddMembers";
    return m_settings->value("Network/GetGroupsAddMembersUrl", "http://localhost:8080/api/user/groupsAddMembers").toString();
}

QString ConfigManager::getSearchUserUrl()const
{
    if (!m_settings) return "http://localhost:8080/api/user/search";
    return m_settings->value("Network/SearchUserUrl", "http://localhost:8080/api/user/search").toString();

}

QString ConfigManager::getFriendRequestUrl()const
{
    if (!m_settings) return "http://localhost:8080/api/friend/request";
    return m_settings->value("Network/FriendRequestUrl", "http://192.168.107.119:8080/api/friend/request").toString();
}

QString ConfigManager::getFriendURL()const
{
    if (!m_settings) return "http://localhost:8080/api/v1/grtfriend/%1";
    return m_settings->value("Network/getFriendURL", "http://localhost:8080/api/v1/grtfriend/%1").toString();
}

QString ConfigManager::updateFriendURL()const
{
    if (!m_settings) return "http://localhost:8080/api/v1/putfriend/1%";
    return m_settings->value("Network/updateFriendURL", "http://localhost:8080/api/v1/putfriend/1%").toString();
}

QString ConfigManager::deleteFriendURL()const
{
    if (!m_settings) return "http://localhost:8080/api/v1/delfriend/1%";
    return m_settings->value("Network/deleteFriendURL", "http://192.168.107.119:8080/api/v1/delfriend/1%").toString();
}


int ConfigManager::maxRetry() const
{
    if (!m_settings) return 5;
    return m_settings->value("Network/MaxRetry", 5).toInt();
}

QString ConfigManager::dataSavePath() const
{
    if (!m_settings) return QDir::homePath() + "/YaroWX/data" + "/user_" + currentLoginUserID ;
    return m_settings->value("Data/SavePath", QDir::homePath() + "/YaroWX/data").toString()
           + "/user_"+currentLoginUserID;
}



QString ConfigManager::getPendingRequestsUrl() const
{
    if (!m_settings) return "http://localhost:8080/api/friend/request/pending";
    return m_settings->value("Network/PendingRequestsUrl", "http://localhost:8080/api/friend/request/pending").toString();
}

QString ConfigManager::processRequestUrl() const
{
    if (!m_settings) return "http://localhost:8080/api/friend/request/{1%}";
    return m_settings->value("Network/processRequestUrl", "http://localhost:8080/api/friend/request/{1%}").toString();
}


void ConfigManager::saveConfig()
{
    if (m_settings) {
        m_settings->sync(); // 确保配置保存到磁盘
    }
}

