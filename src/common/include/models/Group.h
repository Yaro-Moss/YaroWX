#pragma once
#include "ORM_Macros.h"
#include <QObject>

class Group {
    Q_GADGET
    ORM_MODEL(Group, "groups")

    ORM_FIELD(qint64, group_id)           // INTEGER PRIMARY KEY
    ORM_FIELD(QString, group_name)        // TEXT NOT NULL
    ORM_FIELD(QString, avatar)            // TEXT
    ORM_FIELD(QString, avatar_local_path) // TEXT
    ORM_FIELD(QString, announcement)      // TEXT
    ORM_FIELD(int, max_members)           // INTEGER DEFAULT 500
    ORM_FIELD(QString, group_note)        // TEXT

public:
    Group() = default;
};
