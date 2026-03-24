#pragma once
#include "ORM_Macros.h"
#include "User.h"
#include <QObject>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonvalue.h>

class Contact {
    Q_GADGET
    ORM_MODEL(Contact, "contacts")

    ORM_FIELD(qint64, user_id)            // INTEGER PRIMARY KEY
    ORM_FIELD(QString, remark_name)       // TEXT
    ORM_FIELD(QString, description)       // TEXT
    ORM_FIELD(QString, tags)              // TEXT (JSON)
    ORM_FIELD(QString, phone_note)        // TEXT
    ORM_FIELD(QString, email_note)        // TEXT
    ORM_FIELD(QString, source)            // TEXT
    ORM_FIELD(int, is_starred)            // INTEGER DEFAULT 0
    ORM_FIELD(int, is_blocked)            // INTEGER DEFAULT 0
    ORM_FIELD(qint64, add_time)           // INTEGER (时间戳)

public:
    User user = User();
    Contact() = default;

    bool isValid() const{
        return m_user_id.isValid();
    }

    QString getTagsString() {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(tagsValue().toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "JSON解析失败：" << parseError.errorString();
            qDebug()<<tags();
            return "";
        }

        if (!doc.isArray()) {
            qWarning() << "输入不是JSON数组格式！";
            return "";
        }

        QJsonArray tagsArray = doc.array();
        QStringList tagsList; // 用于拼接的字符串列表

        for (const QJsonValue& value : std::as_const(tagsArray)) {
            if (value.isString()) {
                tagsList.append(value.toString());
            } else {
                qWarning() << "数组中包含非字符串元素，已忽略";
            }
        }

        return tagsList.join(",");
    }


};
