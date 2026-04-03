#ifndef FRIENDREQUESTDATA_H
#define FRIENDREQUESTDATA_H

#include <QString>
#include <qglobal.h>

struct FriendRequestItem {
    qint64 id = -1;             // 申请ID
    qint64 fromUserId = -1;     // 申请人ID
    QString message;            // 申请留言
    int status = 0;             // 0=待处理
    QString createdAt;          // 申请时间
    QString fromNickname;       // 申请人昵称
    QString fromAvatar;         // 申请人头像
    QString fromAccount;        // 申请人账号
};

#endif // FRIENDREQUESTDATA_H