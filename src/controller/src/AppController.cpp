#include "AppController.h"

AppController::AppController(DatabaseManager *databaseManager, QObject *parent)
    : QObject(parent)
{
    m_userController = new UserController(databaseManager, this);
    m_conversationController = new ConversationController(databaseManager, this);
    m_messageController = new MessageController(databaseManager, this);
    m_contactController = new ContactController(databaseManager, this);
    m_localMomentController = new LocalMomentController(databaseManager, this);
}

AppController::~AppController(){
    m_userController->deleteLater();
    m_conversationController->deleteLater();
    m_messageController->deleteLater();
    m_contactController->deleteLater();
    m_localMomentController->deleteLater();
}
