#include "network.h"
#include "LoginManager.h"
#include "NetworkDataLoader.h"

Network::Network(QObject *parent)
    : QObject(parent)
{
    m_loginManager = new LoginManager(this);
    m_dataLoader = new NetworkDataLoader(m_loginManager, this);

}
 
Network::~Network()
{
    m_dataLoader->deleteLater();
    m_loginManager->deleteLater();

}