#include "DatabaseInitializationController.h"
#include "Contact.h"
#include "DatabaseInitializer.h"
#include "NetworkDataLoader.h"
#include "LoginManager.h"
#include "ORM.h"
#include "User.h"
#include <QDebug>
#include <QMessageBox>
#include "ORM.h"
#include "Group.h"
#include "GroupMember.h"
#include "network.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

DatabaseInitializationController::DatabaseInitializationController(Network * network, QObject *parent)
    : QObject(parent)
    , m_initializationInProgress(false)
    , m_currentProgress(0)
    , databaseInitializer(new DatabaseInitializer(this))
    , m_networkLoader(network->networkDataLoader())
    , m_loginManager(network->loginManager())
    , m_currentUserId(0)   // 初始化当前用户ID为0
{}

DatabaseInitializationController::~DatabaseInitializationController()
{}

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
        completeInitialization(false, "数据库初始化失败");
        return;
    }
    updateProgress(10, "数据库初始化完成");

    // 步骤1：加载用户信息
    if (!stepLoadUserInfo()) {
        completeInitialization(false, "加载用户信息失败");
        return;
    }
    updateProgress(40, "用户信息加载完成");

    // 步骤2：加载好友列表
    if (!stepLoadFriends()) {
        completeInitialization(false, "加载好友列表失败");
        return;
    }
    updateProgress(70, "好友列表加载完成");

    // 步骤3：加载群组列表
    if (!stepLoadGroupsAddMembers()) {
        completeInitialization(false, "加载群组列表失败");
        return;
    }
    updateProgress(90, "群组成员加载完成");

    // 全部成功
    completeInitialization(true);
}

bool DatabaseInitializationController::stepLoadUserInfo()
{
    QString errorMsg;
    if (!m_networkLoader->loadUserInfo(m_userInfo, errorMsg)) {
        qWarning() << "loadUserInfo failed:" << errorMsg;
        return false;
    }
    // 保存到数据库
    if (!saveUserInfoToDb(m_userInfo)) {
        qWarning() << "保存用户信息到数据库失败";
        return false;
    }
    // 记录当前用户ID
    m_currentUserId = m_userInfo["user_id"].toInteger();
    return true;
}

bool DatabaseInitializationController::stepLoadFriends()
{
    QString errorMsg;
    if (!m_networkLoader->loadFriends(m_friendsArray, errorMsg)) {
        qWarning() << "loadFriends failed:" << errorMsg;
        return false;
    }
    if (!saveFriendsToDb(m_friendsArray)) {
        qWarning() << "保存好友列表到数据库失败";
        return false;
    }
    return true;
}

bool DatabaseInitializationController::stepLoadGroupsAddMembers()
{
    QString errorMsg;
    if (!m_networkLoader->loadGroupsAddMembers(m_groupsArray, errorMsg)) {
        qWarning() << "loadGroups failed:" << errorMsg;
        return false;
    }
    if (!saveGroupsAddMembersToDb(m_groupsArray)) {
        qWarning() << "保存群组列表到数据库失败";
        return false;
    }
    return true;
}

void DatabaseInitializationController::completeInitialization(bool success, const QString &errorMsg)
{
    m_initializationInProgress = false;
    if (success) {
        updateProgress(100, "初始化完成!");
        emit initializationFinished(true);
        emit databaseReady();
    } else {
        updateProgress(m_currentProgress, "初始化失败: " + errorMsg);
        emit initializationFinished(false, errorMsg);
    }
}

void DatabaseInitializationController::updateProgress(int progress, const QString& message)
{
    m_currentProgress = progress;
    emit initializationProgress(progress, message);
}

// 以下为数据存储函数
bool DatabaseInitializationController::saveUserInfoToDb(const QJsonObject &userInfo) {
    // 1. 从 JSON 中提取字段
    qint64 userId = userInfo["user_id"].toInteger();
    QString account = userInfo["account"].toString();
    QString nickname = userInfo["nickname"].toString();
    QString avatar = userInfo["avatar"].toString();
    QString avatarLocalPath = userInfo["avatar_local_path"].toString();
    QString profileCover = userInfo["profile_cover"].toString();
    int gender = userInfo["gender"].toInt();
    QString region = userInfo["region"].toString();
    QString signature = userInfo["signature"].toString();
    int isCurrent = userInfo["is_current"].toInt();

    // 2. 使用 Orm 操作数据库
    Orm orm;

    // 3. 查询当前用户是否已存在
    auto existing = orm.findById<User>(userId);
    if (existing.has_value()) {
        // 已存在，更新记录
        User user = existing.value();
        user.setaccount(account);
        user.setnickname(nickname);
        user.setavatar(avatar);
        user.setavatar_local_path(avatarLocalPath);
        user.setprofile_cover(profileCover);
        user.setgender(gender);
        user.setregion(region);
        user.setsignature(signature);
        user.setis_current(isCurrent);
        return orm.update(user);
    } else {
        // 不存在，插入新记录
        User user;
        user.setuser_id(userId);
        user.setaccount(account);
        user.setnickname(nickname);
        user.setavatar(avatar);
        user.setavatar_local_path(avatarLocalPath);
        user.setprofile_cover(profileCover);
        user.setgender(gender);
        user.setregion(region);
        user.setsignature(signature);
        user.setis_current(isCurrent);
        return orm.insert(user);
    }
}

bool DatabaseInitializationController::saveFriendsToDb(const QJsonArray &friendsArray)
{
    Orm orm;
    if (!orm.transaction()) {
        qCritical() << "saveFriendsToDb: failed to begin transaction";
        return false;
    }

    for (const QJsonValue &value : friendsArray) {
        QJsonObject friendObj = value.toObject();

        // 1. 提取好友用户信息
        QJsonObject userObj = friendObj["user"].toObject();
        qint64 friendUserId = userObj["user_id"].toInteger();
        if (friendUserId == 0) {
            qWarning() << "saveFriendsToDb: invalid friend user_id, skip";
            continue;
        }

        // 构造 User 对象
        User friendUser;
        friendUser.setuser_id(friendUserId);
        friendUser.setaccount(userObj["account"].toString());
        friendUser.setnickname(userObj["nickname"].toString());
        friendUser.setavatar(userObj["avatar"].toString());
        friendUser.setavatar_local_path("");
        friendUser.setprofile_cover(userObj["profile_cover"].toString());
        friendUser.setgender(userObj["gender"].toInt());
        friendUser.setregion(userObj["region"].toString());
        friendUser.setsignature(userObj["signature"].toString());
        friendUser.setis_current( (friendUserId == m_currentUserId) ? 1 : 0 );

        // 保存或更新用户记录
        auto existingUser = orm.findById<User>(friendUserId);
        if (existingUser.has_value()) {
            if (!orm.update(friendUser)) {
                qCritical() << "saveFriendsToDb: failed to update user" << friendUserId;
                orm.rollback();
                return false;
            }
        } else {
            if (!orm.insert(friendUser)) {
                qCritical() << "saveFriendsToDb: failed to insert user" << friendUserId;
                orm.rollback();
                return false;
            }
        }

        // 2. 提取好友关系信息
        Contact contact;
        contact.setuser_id(friendUserId);                     // contacts 表主键
        contact.setremark_name(friendObj["remark"].toString());
        contact.setdescription(friendObj["description"].toString());
        contact.settags(friendObj["tags"].toString());        // JSON 字符串直接存储
        contact.setphone_note(friendObj["phone_note"].toString());
        contact.setemail_note(friendObj["email_note"].toString());
        contact.setsource(friendObj["source"].toString());
        contact.setis_starred(friendObj["is_starred"].toInt());
        contact.setis_blocked(friendObj["is_blocked"].toInt());

        // 转换添加时间（服务端返回的 created_at 格式 "yyyy-MM-dd HH:mm:ss"）
        qint64 addTime = 0;
        QString createdAtStr = friendObj["created_at"].toString();
        if (!createdAtStr.isEmpty()) {
            QDateTime dt = QDateTime::fromString(createdAtStr, "yyyy-MM-dd HH:mm:ss");
            if (dt.isValid()) {
                addTime = dt.toSecsSinceEpoch();
            } else {
                qWarning() << "saveFriendsToDb: invalid created_at format:" << createdAtStr;
            }
        }
        contact.setadd_time(addTime);

        // 保存或更新联系人记录
        auto existingContact = orm.findById<Contact>(friendUserId);
        if (existingContact.has_value()) {
            if (!orm.update(contact)) {
                qCritical() << "saveFriendsToDb: failed to update contact" << friendUserId;
                orm.rollback();
                return false;
            }
        } else {
            if (!orm.insert(contact)) {
                qCritical() << "saveFriendsToDb: failed to insert contact" << friendUserId;
                orm.rollback();
                return false;
            }
        }
    }

    if (!orm.commit()) {
        qCritical() << "saveFriendsToDb: failed to commit transaction";
        return false;
    }

    qDebug() << "saveFriendsToDb: saved" << friendsArray.size() << "friends";
    return true;
}

bool DatabaseInitializationController::saveGroupsAddMembersToDb(const QJsonArray &groupsArray) {
    Orm orm;
    if (!orm.transaction()) {
        qCritical() << "saveGroupsAddMembersToDb: failed to begin transaction";
        return false;
    }

    // 辅助函数：将 "YYYY-MM-DD HH:MM:SS" 格式的时间字符串转换为 Unix 时间戳（秒）
    auto dateTimeToTimestamp = [](const QString &dateTimeStr) -> qint64 {
        if (dateTimeStr.isEmpty()) return 0;
        QDateTime dt = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd HH:mm:ss");
        return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
    };

    for (const QJsonValue &val : groupsArray) {
        QJsonObject groupObj = val.toObject();

        // 1. 保存群组信息（Group 表）
        qint64 groupId = groupObj["group_id"].toInteger();
        if (groupId == 0) {
            qWarning() << "saveGroupsAddMembersToDb: invalid group_id, skip";
            continue;
        }

        Group group;
        group.setgroup_id(groupId);
        group.setgroup_name(groupObj["group_name"].toString());
        group.setavatar(groupObj["avatar"].toString());
        // avatar_local_path 暂不设置
        group.setavatar_local_path("");
        group.setannouncement(groupObj["announcement"].toString());
        group.setmax_members(groupObj["max_members"].toInt(500));
        group.setgroup_note(""); // 群备注暂不设置

        auto existingGroup = orm.findById<Group>(groupId);
        if (existingGroup.has_value()) {
            if (!orm.update(group)) {
                qCritical() << "saveGroupsAddMembersToDb: update group failed, group_id=" << groupId;
                orm.rollback();
                return false;
            }
        } else {
            if (!orm.insert(group)) {
                qCritical() << "saveGroupsAddMembersToDb: insert group failed, group_id=" << groupId;
                orm.rollback();
                return false;
            }
        }

        // 2. 处理该群的成员列表
        QJsonArray membersArray = groupObj["members"].toArray();
        for (const QJsonValue &memberVal : membersArray) {
            QJsonObject memberObj = memberVal.toObject();

            // 提取成员字段
            qint64 userId = memberObj["user_id"].toInteger();
            if (userId == 0) continue;

            // 2.1 保存用户信息（User 表）
            User user;
            user.setuser_id(userId);
            user.setaccount(memberObj["account"].toString());
            // 全局昵称优先使用 global_nickname，若为空则使用 nickname（群内昵称）作为后备
            QString globalNick = memberObj["global_nickname"].toString();
            if (globalNick.isEmpty()) {
                globalNick = memberObj["nickname"].toString();
            }
            user.setnickname(globalNick);
            user.setavatar(memberObj["avatar"].toString());
            user.setgender(0);               // 性别未知
            user.setis_current( (userId == m_currentUserId) ? 1 : 0 );

            auto existingUser = orm.findById<User>(userId);
            if (existingUser.has_value()) {
                if (!orm.update(user)) {
                    qCritical() << "saveGroupsAddMembersToDb: update user failed, user_id=" << userId;
                    orm.rollback();
                    return false;
                }
            } else {
                if (!orm.insert(user)) {
                    qCritical() << "saveGroupsAddMembersToDb: insert user failed, user_id=" << userId;
                    orm.rollback();
                    return false;
                }
            }

            // 2.2 保存群成员关系（GroupMember 表）
            GroupMember member;
            member.setgroup_id(groupId);
            member.setuser_id(userId);
            member.setnickname(memberObj["nickname"].toString());  // 群内昵称
            member.setrole(memberObj["role"].toInt());             // 角色
            member.setjoin_time(dateTimeToTimestamp(memberObj["join_time"].toString()));
            bool isContact = false;
            auto existingContact = orm.findById<Contact>(userId);
            if (existingContact.has_value()) {
                isContact = true;
            }
            member.setis_contact(isContact ? 1 : 0);

            // 检查成员关系是否已存在（由于主键为 (group_id, user_id)，可直接尝试更新）
            QString where = "group_id = ? AND user_id = ?";
            QVariantList params = { groupId, userId };
            auto existingMemberList = orm.findWhere<GroupMember>(where, params, "", -1, -1);
            if (!existingMemberList.isEmpty()) {
                // 已存在，更新
                if (!orm.update(member)) {
                    qCritical() << "saveGroupsAddMembersToDb: update group_member failed, group_id=" << groupId << ", user_id=" << userId;
                    orm.rollback();
                    return false;
                }
            } else {
                // 不存在，插入
                if (!orm.insert(member)) {
                    qCritical() << "saveGroupsAddMembersToDb: insert group_member failed, group_id=" << groupId << ", user_id=" << userId;
                    orm.rollback();
                    return false;
                }
            }
        }
    }

    if (!orm.commit()) {
        qCritical() << "saveGroupsAddMembersToDb: commit transaction failed";
        return false;
    }

    qDebug() << "saveGroupsAddMembersToDb: saved" << groupsArray.size() << "groups and their members";
    return true;
}
