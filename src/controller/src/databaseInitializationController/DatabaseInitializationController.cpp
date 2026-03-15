#include "DatabaseInitializationController.h"
#include "DatabaseInitializer.h"
// #include "NetworkManager.h"  // 假设有网络管理器
#include <QDebug>

DatabaseInitializationController::DatabaseInitializationController(QObject *parent)
    : QObject(parent)
    , m_initializationInProgress(false)
    , m_currentProgress(0)
    , databaseInitializer(new DatabaseInitializer(this))
{
}

DatabaseInitializationController::~DatabaseInitializationController()
{
}

void DatabaseInitializationController::initialize()
{
    if (m_initializationInProgress) {
        qWarning() << "Database initialization is already in progress";
        return;
    }

    m_initializationInProgress = true;
    m_currentProgress = 0;

    updateProgress(0, "开始初始化...");
    bool success = databaseInitializer->ensureInitialized();
    if (!success) {
        m_initializationInProgress = false;
        emit initializationFinished(false, "Database initialization failed immediately");
        return;
    }
    updateProgress(30, "数据库初始化完成，开始加载网络数据...");
    loadNetworkData();

}

void DatabaseInitializationController::loadNetworkData()
{
    // 这里调用网络管理器加载各种数据
    updateProgress(40, "加载当前用户信息...");
    loadCurrentUserInfo();
}

void DatabaseInitializationController::loadCurrentUserInfo()
{
    // 模拟从网络加载当前用户信息
    // NetworkManager* networkManager = NetworkManager::getInstance(); // 假设的网络管理器

    // 这里应该是异步调用，为了示例简化处理
    // 实际中应该连接网络请求的信号
    // bool success = networkManager->loadCurrentUser();

    bool success = true;
    if (success) {
        updateProgress(50, "用户信息加载完成，加载联系人...");
        loadContacts();
    } else {
        m_initializationInProgress = false;
        emit initializationFinished(false, "Failed to load current user info");
    }
}

void DatabaseInitializationController::loadContacts()
{
    // // 模拟从网络加载联系人
    // NetworkManager* networkManager = NetworkManager::getInstance();
    // bool success = networkManager->loadContacts();
    bool success = true;

    if (success) {
        updateProgress(60, "联系人加载完成，加载群组...");
        loadGroups();
    } else {
        m_initializationInProgress = false;
        emit initializationFinished(false, "Failed to load contacts");
    }
}

void DatabaseInitializationController::loadGroups()
{
    // // 模拟从网络加载群组
    // NetworkManager* networkManager = NetworkManager::getInstance();
    // bool success = networkManager->loadGroups();
    bool success = true;

    if (success) {
        updateProgress(75, "群组加载完成，加载群成员...");
        loadGroupMembers();
    } else {
        m_initializationInProgress = false;
        emit initializationFinished(false, "Failed to load groups");
    }
}

void DatabaseInitializationController::loadGroupMembers()
{
    // // 模拟从网络加载群成员
    // NetworkManager* networkManager = NetworkManager::getInstance();
    // bool success = networkManager->loadGroupMembers();
    bool success = true;

    if (success) {
        updateProgress(90, "群成员加载完成，完成初始化...");
        completeInitialization();
    } else {
        m_initializationInProgress = false;
        emit initializationFinished(false, "Failed to load group members");
    }
}

void DatabaseInitializationController::completeInitialization()
{
    m_initializationInProgress = false;
    updateProgress(100, "初始化完成!");

    emit initializationFinished(true);
    emit databaseReady();
}

void DatabaseInitializationController::updateProgress(int progress, const QString& message)
{
    m_currentProgress = progress;
    emit initializationProgress(progress, message);
}
