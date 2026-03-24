#include "AppController.h"

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    m_userController = new UserController(this);
    m_conversationController = new ConversationController(this);
    m_messageController = new MessageController(this);
    m_contactController = new ContactController(this);
    m_localMomentController = new LocalMomentController(this);
}

AppController::~AppController(){
    m_userController->deleteLater();
    m_conversationController->deleteLater();
    m_messageController->deleteLater();
    m_contactController->deleteLater();
    m_localMomentController->deleteLater();
}
