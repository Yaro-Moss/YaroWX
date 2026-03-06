#ifndef CONTACT_H
#define CONTACT_H

#include <QJsonObject>
#include <QtSql/QSqlQuery>
#include <QString>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include "User.h"

struct Contact {
    qint64 userId = -1;
    QString remarkName;
    QString description;
    QJsonArray tags;
    QString phoneNote;
    QString emailNote;
    QString source;
    bool isStarred = false;
    bool isBlocked = false;
    qint64 addTime = 0;

    User user=User();

    Contact() = default;
    
    explicit Contact(const QSqlQuery& query) {
        userId = query.value("user_id").toLongLong();
        remarkName = query.value("remark_name").toString();
        description = query.value("description").toString();
        
        // 解析 tags 字段
        QString tagsJson = query.value("tags").toString();
        if (!tagsJson.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(tagsJson.toUtf8());
            if (doc.isArray()) {
                tags = doc.array();
            }
        }
        
        phoneNote = query.value("phone_note").toString();
        emailNote = query.value("email_note").toString();
        source = query.value("source").toString();
        isStarred = query.value("is_starred").toBool();
        isBlocked = query.value("is_blocked").toBool();
        addTime = query.value("add_time").toLongLong();
    }

    QJsonObject toJson() const {
        return {
            {"user_id", QString::number(userId)},
            {"remark_name", remarkName},
            {"description", description},
            {"tags", tags},
            {"phone_note", phoneNote},
            {"email_note", emailNote},
            {"source", source},
            {"is_starred", isStarred},
            {"is_blocked", isBlocked},
            {"add_time", addTime},
        };
    }

    static Contact fromJson(const QJsonObject& json) {
        Contact contact;
        contact.userId = json["user_id"].toString().toLongLong();
        contact.remarkName = json["remark_name"].toString();
        contact.description = json["description"].toString();
        contact.tags = json["tags"].toArray();
        contact.phoneNote = json["phone_note"].toString();
        contact.emailNote = json["email_note"].toString();
        contact.source = json["source"].toString();
        contact.isStarred = json["is_starred"].toBool();
        contact.isBlocked = json["is_blocked"].toBool();
        contact.addTime = json["add_time"].toVariant().toLongLong();
        return contact;
    }

    bool isValid() const {
        return userId > 0;
    }

    // 便捷方法
    QString getDisplayName() const {
        return !remarkName.isEmpty() ? remarkName : QString::number(userId);
    }

    bool hasTags() const {
        return !tags.isEmpty();
    }

    bool hasPhoneNote() const {
        return !phoneNote.isEmpty();
    }

    bool hasEmailNote() const {
        return !emailNote.isEmpty();
    }

    QString getTagsString() const {
        QStringList tagList;
        for (const QJsonValue& tag : tags) {
            tagList.append(tag.toString());
        }
        return tagList.join(", ");
    }

    void addTag(const QString& tag) {
        if (!tags.contains(tag)) {
            tags.append(tag);
        }
    }

    void removeTag(const QString& tag) {
        QJsonArray newTags;
        for (const QJsonValue& value : tags) {
            if (value.toString() != tag) {
                newTags.append(value);
            }
        }
        tags = newTags;
    }
};

Q_DECLARE_METATYPE(Contact)


#endif // CONTACT_H
