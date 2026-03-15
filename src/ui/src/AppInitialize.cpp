#include "AppInitialize.h"
#include "DatabaseInitializationController.h"
#include "SplashDialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QTimer>

AppInitialize::AppInitialize(DatabaseInitializationController* initController,
                             QObject *parent)
    : QObject(parent)
    , m_splashDialog(nullptr)
    , m_initController(initController)
{
}

bool AppInitialize::initialize()
{
    // 创建并显示启动界面
    m_splashDialog = new SplashDialog();
    m_splashDialog->show();

    connect(m_initController, &DatabaseInitializationController::initializationProgress,
            this, &AppInitialize::onDatabaseInitializationProgress);
    connect(m_initController, &DatabaseInitializationController::initializationFinished,
            this, &AppInitialize::onDatabaseInitializationFinished);
    connect(m_initController, &DatabaseInitializationController::databaseReady,
            this, &AppInitialize::onDatabaseReady);

    // 启动数据库初始化
    QTimer::singleShot(200, m_initController, &DatabaseInitializationController::initialize);

    return true;
}

void AppInitialize::onDatabaseInitializationProgress(int progress, const QString& message)
{
    qDebug() << "Database initialization progress:" << progress << "% -" << message;
    if (m_splashDialog) {
        m_splashDialog->setProgress(progress, message);
    }
}

void AppInitialize::onDatabaseInitializationFinished(bool success, const QString& errorMessage)
{
    if (success) {
        if (m_splashDialog) {
            m_splashDialog->setProgress(100, "数据初始化完成!");
        }
    } else {
        if (m_splashDialog) {
            m_splashDialog->setProgress(0, "数据初始化失败!");
        }
        QMessageBox::critical(nullptr, "Database Error",
                              "Failed to initialize database: " + errorMessage);
        QApplication::exit(1);
    }
}

void AppInitialize::onDatabaseReady()
{
    if (m_splashDialog) {
        m_splashDialog->setProgress(100, "应用程序准备就绪!");
        m_splashDialog->hide();
        m_splashDialog->deleteLater();
        m_splashDialog = nullptr;
        emit isInited();
    }
}


