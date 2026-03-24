#ifndef USERCONTROLLER_H
#define USERCONTROLLER_H

#include <QObject>
#include "Contact.h"
#include "User.h"

class UserController : public QObject
{
    Q_OBJECT

public:
    explicit UserController(QObject* parent = nullptr);
    ~UserController();

    // 设置当前登录用户（测试用，正常应通过登录流程获取）
    void setCurrentLoginUser(const Contact& user) { m_currentLoginUser = user; }

    // 用户管理操作（异步）
    Q_INVOKABLE void addUser(const User& user);
    Q_INVOKABLE void updateUser(const User& user);
    Q_INVOKABLE void deleteUser(qint64 userId);
    Q_INVOKABLE void getUser(qint64 userId);
    Q_INVOKABLE void getAllUsers();

    // 当前登录用户的快捷更新方法
    Q_INVOKABLE void updateNickname(const QString& nickname);
    Q_INVOKABLE void updateAvatar(const QString& avatarUrl, const QString& localPath = QString());
    Q_INVOKABLE void updateSignature(const QString& signature);
    Q_INVOKABLE void updateGender(int gender);
    Q_INVOKABLE void updateRegion(const QString& region);

signals:
    // 单用户操作结果信号
    void userLoaded(qint64 userId, const User& user);
    void userSaved(qint64 userId, bool success, const QString& errorMsg = QString());
    void userUpdated(qint64 userId);
    void userDeleted(qint64 userId);
    void allUsersLoaded(const QList<User>& users);
    void currentUserUpdated();  // 当前登录用户信息已更新

private:
    Contact m_currentLoginUser;  // 当前登录用户（含联系人信息）
};

#endif // USERCONTROLLER_H
