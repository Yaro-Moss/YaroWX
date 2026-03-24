#ifndef GROUPCONTROLLER_H
#define GROUPCONTROLLER_H

#include <QObject>
#include <QMap>
#include <QAtomicInt>
#include "Contact.h"
#include "Group.h"

class GroupController : public QObject
{
    Q_OBJECT

public:
    explicit GroupController(QObject* parent = nullptr);
    ~GroupController();

    void setCurrentLoginUser(const Contact &user){m_currentLoginUser = user;};

private:

    Contact m_currentLoginUser;


};

#endif // GROUPCONTROLLER_H
