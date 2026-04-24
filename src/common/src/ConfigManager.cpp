#include "ConfigManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

static QString joinUrl(const QString& base, const QString& rel)
{
    QString relPath = rel;
    if (relPath.startsWith("http://", Qt::CaseInsensitive) ||
        relPath.startsWith("https://", Qt::CaseInsensitive)) {
        return relPath;
    }
    if (relPath.startsWith('/')) {
        relPath = relPath.mid(1);
    }
    if (base.endsWith('/')) {
        return base + relPath;
    } else {
        return base + '/' + relPath;
    }
}

static QString buildUrl(const QSettings* settings, const QString& key,
                        const QString& defaultRelPath, const QString& baseUrl)
{
    QString path = settings ? settings->value(key, defaultRelPath).toString() : defaultRelPath;
    return joinUrl(baseUrl, path);
}

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
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
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
        out << "baseUrl=http://localhost:8080\n";
        out << "WebSocketUrl=ws://localhost:8080/chat\n";
        out << "LoginUrl=/api/v1/login\n";
        out << "RegisterUrl=/api/v1/register\n";
        out << "GetAllFriendUrl=/api/v1/friend\n";
        out << "GetProfileUrl=/api/user/profile\n";
        out << "GetGroupsAddMembersUrl=/api/user/groupsAddMembers\n";
        out << "SearchUserUrl=/api/user/search\n";
        out << "FriendRequestUrl=/api/friend/request\n";
        out << "PendingRequestsUrl=/api/friend/request/pending\n";
        out << "processRequestUrl=/api/friend/request/%1\n";
        out << "getFriendURL=/api/v1/grtfriend/%1\n";
        out << "updateFriendURL=/api/v1/putfriend/%1\n";
        out << "deleteFriendURL=/api/v1/delfriend/%1\n";
        out << "UploadUrl=/api/v1/upload\n";   // 注意：默认改为 /api/v1/upload
        out << "MaxRetry=5\n\n";

        out << "[Data]\n";
        out << "SavePath=" << QDir::homePath() + "/YaroWX/data" << "\n";

        configFile.close();
    }
}

QString ConfigManager::baseUrl() const
{
    if (!m_settings) return "http://localhost:8080";
    return m_settings->value("Network/baseUrl", "http://localhost:8080").toString();
}

QString ConfigManager::webSocketUrl() const
{
    if (!m_settings) return "ws://localhost:8080/chat";
    return m_settings->value("Network/WebSocketUrl", "ws://localhost:8080/chat").toString();
}

QString ConfigManager::loginUrl() const
{
    return buildUrl(m_settings, "Network/LoginUrl", "/api/v1/login", baseUrl());
}

QString ConfigManager::registerUrl() const
{
    return buildUrl(m_settings, "Network/RegisterUrl", "/api/v1/register", baseUrl());
}

QString ConfigManager::getAllFriendUrl() const
{
    return buildUrl(m_settings, "Network/GetAllFriendUrl", "/api/v1/friend", baseUrl());
}

QString ConfigManager::getProfileUrl() const
{
    return buildUrl(m_settings, "Network/GetProfileUrl", "/api/user/profile", baseUrl());
}

QString ConfigManager::getGroupsAddMembersUrl() const
{
    return buildUrl(m_settings, "Network/GetGroupsAddMembersUrl", "/api/user/groupsAddMembers", baseUrl());
}

QString ConfigManager::getSearchUserUrl() const
{
    return buildUrl(m_settings, "Network/SearchUserUrl", "/api/user/search", baseUrl());
}

QString ConfigManager::getFriendRequestUrl() const
{
    return buildUrl(m_settings, "Network/FriendRequestUrl", "/api/friend/request", baseUrl());
}

QString ConfigManager::getPendingRequestsUrl() const
{
    return buildUrl(m_settings, "Network/PendingRequestsUrl", "/api/friend/request/pending", baseUrl());
}

QString ConfigManager::processRequestUrl() const
{
    QString relPath = m_settings ? m_settings->value("Network/processRequestUrl", "/api/friend/request/%1").toString()
                                 : "/api/friend/request/%1";
    return joinUrl(baseUrl(), relPath);
}

QString ConfigManager::getFriendURL() const
{
    QString relPath = m_settings ? m_settings->value("Network/getFriendURL", "/api/v1/grtfriend/%1").toString()
                                 : "/api/v1/grtfriend/%1";
    return joinUrl(baseUrl(), relPath);
}

QString ConfigManager::updateFriendURL() const
{
    QString relPath = m_settings ? m_settings->value("Network/updateFriendURL", "/api/v1/putfriend/%1").toString()
                                 : "/api/v1/putfriend/%1";
    return joinUrl(baseUrl(), relPath);
}

QString ConfigManager::deleteFriendURL() const
{
    QString relPath = m_settings ? m_settings->value("Network/deleteFriendURL", "/api/v1/delfriend/%1").toString()
                                 : "/api/v1/delfriend/%1";
    return joinUrl(baseUrl(), relPath);
}

QString ConfigManager::getUploadUrl() const
{
    return buildUrl(m_settings, "Network/UploadUrl", "/api/v1/upload", baseUrl());
}

int ConfigManager::maxRetry() const
{
    if (!m_settings) return 5;
    return m_settings->value("Network/MaxRetry", 5).toInt();
}

QString ConfigManager::dataSavePath() const
{
    QString basePath;
    if (!m_settings) {
        basePath = QDir::homePath() + "/YaroWX/data";
    } else {
        basePath = m_settings->value("Data/SavePath", QDir::homePath() + "/YaroWX/data").toString();
    }
    return basePath + "/user_" + currentLoginUserID;
}

void ConfigManager::saveConfig()
{
    if (m_settings) {
        m_settings->sync();
    }
}