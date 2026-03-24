#pragma once
#include "ORM_Macros.h"
#include <QObject>


class User {
    Q_GADGET
    ORM_MODEL(User, "users")

    ORM_FIELD(qint64, user_id)            // INTEGER PRIMARY KEY
    ORM_FIELD(QString, account)           // TEXT NOT NULL
    ORM_FIELD(QString, nickname)          // TEXT NOT NULL
    ORM_FIELD(QString, avatar)            // TEXT
    ORM_FIELD(QString, avatar_local_path) // TEXT
    ORM_FIELD(QString, profile_cover)     // TEXT
    ORM_FIELD(int, gender)                // INTEGER DEFAULT 0
    ORM_FIELD(QString, region)            // TEXT
    ORM_FIELD(QString, signature)         // TEXT
    ORM_FIELD(int, is_current)            // INTEGER DEFAULT 0

public:
    User() = default;  // 无需初始化主键，QVariant 默认为空

    bool isValid() const {
        return m_user_id.isValid();
    }
};
