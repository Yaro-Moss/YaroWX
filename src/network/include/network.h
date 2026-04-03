#pragma once

#include <QObject>

class NetworkDataLoader;
class LoginManager;

class Network : public QObject{
public:
    Network(QObject* parent = nullptr);
    ~Network();

    NetworkDataLoader* networkDataLoader(){ return m_dataLoader; }
    LoginManager* loginManager(){ return m_loginManager; }


private:
    NetworkDataLoader* m_dataLoader;
    LoginManager* m_loginManager;
};  
   