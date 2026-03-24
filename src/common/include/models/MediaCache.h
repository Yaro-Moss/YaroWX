#pragma once
#include "ORM_Macros.h"
#include <QObject>

class MediaCache {
    Q_GADGET
    ORM_MODEL(MediaCache, "media_cache")

    ORM_FIELD(qint64, cache_id)            // INTEGER PRIMARY KEY AUTOINCREMENT
    ORM_FIELD(QString, file_path)          // TEXT UNIQUE NOT NULL
    ORM_FIELD(int, file_type)              // INTEGER NOT NULL
    ORM_FIELD(QString, original_url)       // TEXT
    ORM_FIELD(qint64, file_size)           // INTEGER
    ORM_FIELD(int, access_count)           // INTEGER DEFAULT 0
    ORM_FIELD(qint64, last_access_time)    // INTEGER
    ORM_FIELD(qint64, created_time)        // INTEGER

public:
    MediaCache() = default;
};
