#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include "ContactController.h"
#include "ConversationController.h"
#include "MessageController.h"
#include "UserController.h"
#include "LocalMomentController.h"

class Network;

/**
 * @brief 应用程序控制器：封装一组表访问对象。
 */
class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(Network *network, QObject *parent = nullptr);
    ~AppController();
    UserController *userController() const { return m_userController; }
    ConversationController *conversationController() const { return m_conversationController; }
    MessageController *messageController() const { return m_messageController; }
    ContactController *contactController() const{return m_contactController;}
    LocalMomentController* localMomentController(){return m_localMomentController;}

private:
    UserController *m_userController = nullptr;
    ConversationController *m_conversationController = nullptr;
    MessageController *m_messageController = nullptr;
    ContactController* m_contactController = nullptr;
    LocalMomentController* m_localMomentController = nullptr;
};

#endif // APPCONTROLLER_H
