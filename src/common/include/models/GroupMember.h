#pragma once
#include "ORM_Macros.h"
#include <QObject>

class GroupMember {
    Q_GADGET
    ORM_MODEL(GroupMember, "group_members")

    ORM_FIELD(qint64, group_id)
    ORM_FIELD(qint64, user_id)
    ORM_FIELD(QString, nickname)          // TEXT NOT NULL
    ORM_FIELD(int, role)                  // INTEGER DEFAULT 0
    ORM_FIELD(qint64, join_time)          // INTEGER
    ORM_FIELD(int, is_contact)            // INTEGER DEFAULT 0

public:
    GroupMember() = default;
};
