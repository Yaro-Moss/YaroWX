#ifndef USER_H
#define USER_H

#include <QJsonObject>
#include <QtSql/QSqlQuery>
#include <QString>

struct User {
    qint64 userId = 0;
    QString account;
    QString nickname;
    QString avatar;
    QString avatarLocalPath;
    QString profile_cover;
    int gender = 0;
    QString region;
    QString signature;
    bool isCurrent = false;

    User() = default;
    
    explicit User(const QSqlQuery& query) {
        userId = query.value("user_id").toLongLong();
        account = query.value("account").toString();
        nickname = query.value("nickname").toString();
        avatar = query.value("avatar").toString();
        avatarLocalPath = query.value("avatar_local_path").toString();
        profile_cover = query.value("profile_cover").toString();
        gender = query.value("gender").toInt();
        region = query.value("region").toString();
        signature = query.value("signature").toString();
        isCurrent = query.value("is_current").toBool();
    }

    QJsonObject toJson() const {
        return {
            {"user_id", QString::number(userId)},
            {"account", account},
            {"nickname", nickname},
            {"avatar", avatar},
            {"avatar_local_path", avatarLocalPath},
            {"profile_cover", profile_cover},
            {"gender", gender},
            {"region", region},
            {"signature", signature},
            {"is_current", isCurrent}
        };
    }

    static User fromJson(const QJsonObject& json) {
        User user;
        user.userId = json["user_id"].toString().toLongLong();
        user.account = json["account"].toString();
        user.nickname = json["nickname"].toString();
        user.avatar = json["avatar"].toString();
        user.avatarLocalPath = json["avatar_local_path"].toString();
        user.profile_cover = json["profile_cover"].toString();
        user.gender = json["gender"].toInt();
        user.region = json["region"].toString();
        user.signature = json["signature"].toString();
        user.isCurrent = json["is_current"].toBool();
        return user;
    }

    bool isValid() const {
        return userId > 0 && !account.isEmpty() && !nickname.isEmpty();
    }

    // 为了方便使用，添加一些便捷方法
    QString getDisplayName() const {
        return !nickname.isEmpty() ? nickname : account;
    }

    bool isMale() const { return gender == 1; }
    bool isFemale() const { return gender == 2; }
};

Q_DECLARE_METATYPE(User)


#endif // USER_H
