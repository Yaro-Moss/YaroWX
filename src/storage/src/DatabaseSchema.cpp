#include "DatabaseSchema.h"
#include <QStringList>

// 数据库表名常量定义
const char* DatabaseSchema::TABLE_CURRENT_USER = "users";
const char* DatabaseSchema::TABLE_CONTACTS = "contacts";
const char* DatabaseSchema::TABLE_GROUP_MEMBERS = "group_members";
const char* DatabaseSchema::TABLE_GROUPS = "groups";
const char* DatabaseSchema::TABLE_CONVERSATIONS = "conversations";
const char* DatabaseSchema::TABLE_MESSAGES = "messages";
const char* DatabaseSchema::TABLE_MEDIA_CACHE = "media_cache";
const char* DatabaseSchema::TABLE_LOCAL_MOMENT = "local_moment";
const char* DatabaseSchema::TABLE_LOCAL_MOMENT_INTERACT = "local_moment_interact";
const char* DatabaseSchema::TABLE_FRIEND_REQUESTS = "friend_requests";

/**
 * @brief 获取创建"用户表"的SQL语句
 */
QString DatabaseSchema::getCreateTableUser() {
    return R"(
        CREATE TABLE IF NOT EXISTS users (
            user_id INTEGER NOT NULL PRIMARY KEY,    -- 用户ID
            account TEXT NOT NULL,                   -- 用户账号
            nickname TEXT NOT NULL,                  -- 用户昵称
            avatar TEXT,                             -- 头像远程URL
            avatar_local_path TEXT,                  -- 头像本地缓存路径
            profile_cover TEXT,                      -- 朋友圈封面
            gender INTEGER DEFAULT 0,                -- 性别（0:未知 1:男 2:女）
            region TEXT,                             -- 地区（如"中国-北京"）
            signature TEXT,                          -- 个性签名
            is_current INTEGER DEFAULT 0             -- 标记当前登录用户
        )
    )";
}

/**
 * @brief 获取创建"联系人表"的SQL语句
 */
QString DatabaseSchema::getCreateTableContacts() {
    return R"(
        CREATE TABLE IF NOT EXISTS contacts (
            user_id INTEGER NOT NULL PRIMARY KEY,  -- 联系人ID
            remark_name TEXT,                      -- 备注名
            description TEXT,                      -- 描述信息
            tags TEXT,                             -- 标签（JSON）
            phone_note TEXT,                       -- 记录的电话
            email_note TEXT,                       -- 记录的邮箱
            source TEXT,                           -- 添加来源
            is_starred INTEGER DEFAULT 0,          -- 是否星标
            is_blocked INTEGER DEFAULT 0,          -- 是否拉黑
            add_time INTEGER,                      -- 添加时间戳

            -- 外键关联用户表
            FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
        )
    )";
}

/**
 * @brief 获取创建"群成员表"的SQL语句
 */
QString DatabaseSchema::getCreateTableGroupMembers() {
    return R"(
        CREATE TABLE IF NOT EXISTS group_members (
            group_id INTEGER NOT NULL,                   -- 群ID
            user_id INTEGER NOT NULL,                    -- 用户ID

            nickname TEXT NOT NULL,                      -- 群内昵称
            role INTEGER DEFAULT 0,                      -- 群内角色（0:普通 1:管理员 2:群主）
            join_time INTEGER,                           -- 加入时间戳
            is_contact INTEGER DEFAULT 0,                -- 是否为当前用户的联系人

            -- 复合主键：群ID+用户ID唯一标识群成员
            PRIMARY KEY (group_id, user_id),

            -- 外键关联
            FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE,
            FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
        )
    )";
}

/**
 * @brief 获取创建"群组表"的SQL语句
 */
QString DatabaseSchema::getCreateTableGroups() {
    return R"(
        CREATE TABLE IF NOT EXISTS groups (
            group_id INTEGER PRIMARY KEY,                -- 群ID
            group_name TEXT NOT NULL,                    -- 群名称
            avatar TEXT,                                 -- 群头像远程URL
            avatar_local_path TEXT,                      -- 群头像本地缓存路径
            announcement TEXT,                           -- 群公告
            max_members INTEGER DEFAULT 500,             -- 最大成员限制
            group_note TEXT                             -- 群备注
        )
    )";
}

/**
 * @brief 获取创建"会话表"的SQL语句
 */
QString DatabaseSchema::getCreateTableConversations() {
    return R"(
        CREATE TABLE IF NOT EXISTS conversations (
            conversation_id INTEGER PRIMARY KEY AUTOINCREMENT,   -- 会话ID
            group_id INTEGER UNIQUE,                     -- 目标群聊ID（唯一，避免重复群聊会话）
            user_id INTEGER UNIQUE,                      -- 目标用户ID（唯一，避免重复单聊会话）
            type INTEGER NOT NULL,                       -- 会话类型（0:单聊 1:群聊）

            -- 冗余字段（列表展示时快速查询）
            title TEXT,                                  -- 会话标题
            avatar TEXT,                                 -- 会话头像URL
            avatar_local_path TEXT,                      -- 头像本地路径
            last_message_content TEXT,                   -- 最后一条消息内容
            last_message_time INTEGER,                   -- 最后一条消息时间戳

            unread_count INTEGER DEFAULT 0,              -- 未读数量
            is_top INTEGER DEFAULT 0,                    -- 是否置顶

            -- 外键关联
            FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE,
            FOREIGN KEY (user_id) REFERENCES contacts(user_id) ON DELETE CASCADE
        )
    )";
}

/**
 * @brief 获取创建"消息表"的SQL语句
 */
QString DatabaseSchema::getCreateTableMessages() {
    return R"(
        CREATE TABLE IF NOT EXISTS messages (
            message_id INTEGER PRIMARY KEY,              -- 消息ID
            conversation_id INTEGER NOT NULL,            -- 所属会话ID
            sender_id INTEGER NOT NULL,                  -- 发送者用户ID
            consignee_id INTEGER NOT NULL,               -- 接收者用户ID

            type INTEGER NOT NULL,                       -- 消息类型（0:文本 1:图片 2：视频 3：文件 4：语音）
            content TEXT,                                -- 消息内容
            file_path TEXT,                              -- 媒体本地路径
            file_url TEXT,                               -- 媒体远程URL
            file_size INTEGER,                           -- 媒体大小（字节）
            duration INTEGER,                            -- 音视频时长（秒）
            thumbnail_path TEXT,                         -- 缩略图路径
            msg_time INTEGER,                            -- 发送/接收时间戳

            -- 外键关联
            FOREIGN KEY (conversation_id) REFERENCES conversations(conversation_id) ON DELETE CASCADE,
            FOREIGN KEY (sender_id) REFERENCES users(user_id) ON DELETE RESTRICT,
            FOREIGN KEY (consignee_id) REFERENCES users(user_id) ON DELETE RESTRICT
        )
    )";
}

/**
 * @brief 获取创建"媒体缓存表"的SQL语句
 */
QString DatabaseSchema::getCreateTableMediaCache() {
    return R"(
        CREATE TABLE IF NOT EXISTS media_cache (
            cache_id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 本地自增ID
            file_path TEXT UNIQUE NOT NULL,              -- 本地文件路径（唯一）
            file_type INTEGER NOT NULL,                  -- 文件类型
            original_url TEXT,                           -- 原始URL
            file_size INTEGER,                           -- 文件大小
            access_count INTEGER DEFAULT 0,              -- 访问次数
            last_access_time INTEGER,                    -- 最后访问时间
            created_time INTEGER                         -- 创建时间
        )
    )";
}

/**
 * @brief 获取创建朋友圈主表的SQL语句
 * 缓存朋友圈核心信息，合并服务端moment+moment_image逻辑，新增本地缓存字段
 */
QString DatabaseSchema::getCreateTableLocalMoment() {
    return R"(
        CREATE TABLE IF NOT EXISTS local_moment (
            moment_id INTEGER NOT NULL PRIMARY KEY,      -- 服务端朋友圈唯一ID（主键，用于同步）
            user_id INTEGER NOT NULL,                    -- 发布者ID
            username TEXT NOT NULL,                      -- 发布者昵称（本地缓存）
            avatar_local_path TEXT,                      -- 发布者头像本地路径
            avatar_url TEXT,                             -- 发布者头像网络URL
            content TEXT DEFAULT '',                     -- 文本内容

            -- 视频相关（与图片互斥）
            video_local_path TEXT DEFAULT '',            -- 视频本地存储路径
            video_url TEXT DEFAULT '',                   -- 视频网络URL
            video_download_status INTEGER DEFAULT 0,     -- 视频下载状态：0-未下载 1-下载中 2-已下载 3-下载失败

            -- 图片相关：JSON字符串存储[{"url":"xxx","local_path":"xxx","download_status":0},...]
            images_info TEXT DEFAULT '[]',               -- 图片信息列表

            -- 权限与状态（同步服务端）
            privacy_type INTEGER DEFAULT 0,              -- 权限类型：0-公开 1-仅好友可见 2-仅部分可见 3-不给谁看
            is_deleted INTEGER DEFAULT 0,                -- 是否删除（同步服务端软删除）

            -- 本地缓存控制字段
            sync_status INTEGER DEFAULT 0,               -- 同步状态：0-已同步 1-待同步 2-同步失败
            expire_time INTEGER,                         -- 缓存过期时间戳（30天）
            create_time INTEGER,                         -- 服务端发布时间戳
            local_update_time INTEGER DEFAULT (strftime('%s', 'now')), -- 本地更新时间戳

            -- 外键关联用户表
            FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
        )
    )";
}

/**
 * @brief 获取创建朋友圈互动表的SQL语句
 * 缓存点赞、评论等互动数据，避免频繁查询
 */
QString DatabaseSchema::getCreateTableLocalMomentInteract() {
    return R"(
        CREATE TABLE IF NOT EXISTS local_moment_interact (
            interact_id INTEGER PRIMARY KEY AUTOINCREMENT, -- 本地自增ID
            moment_id INTEGER NOT NULL UNIQUE,             -- 关联朋友圈ID（UNIQUE约束确保唯一）
            likes TEXT DEFAULT '[]',                       -- 点赞列表JSON：[{"user_id":1,"username":"xxx","avatar_local_path":"xxx"},...]
            comments TEXT DEFAULT '[]',                    -- 评论列表JSON：[{"comment_id":1,"user_id":1,"content":"xxx","images_info":"[]","reply_user_id":0,"create_time":123456},...]
            local_update_time INTEGER DEFAULT (strftime('%s', 'now')), -- 本地更新时间戳

            -- 外键关联朋友圈主表
            FOREIGN KEY (moment_id) REFERENCES local_moment(moment_id) ON DELETE CASCADE
        )
    )";
}

/**
 * @brief 获取好友请求表创建语句
 */
QString DatabaseSchema::getCreateTableFriendRequests() {
    return R"(
        CREATE TABLE IF NOT EXISTS friend_requests (
            -- 服务端返回字段（严格匹配）
            id INTEGER PRIMARY KEY,                     -- 申请ID
            from_user_id INTEGER NOT NULL,              -- 申请人ID
            message TEXT,                               -- 申请附言
            status INTEGER NOT NULL DEFAULT 0,          -- 0:待处理 1:已同意 2:已拒绝
            created_at INTEGER,                         -- 申请发起时间戳（Unix秒数）
            from_nickname TEXT,                         -- 申请人昵称
            from_avatar TEXT,                           -- 申请人头像URL
            from_account TEXT,                          -- 申请人账号

            -- 客户端本地扩展字段（仅用于 UI 状态）
            is_read INTEGER DEFAULT 0,                  -- 0:未读 1:已读
            local_processed INTEGER DEFAULT 0           -- 0:未处理 1:已处理（防重复）
        )
    )";
}

/**
 * @brief 获取创建消息表触发器的SQL语句
 * 用于在插入、更新、删除消息时自动更新会话的最后消息
 */
QStringList DatabaseSchema::getCreateTriggers()
{
    return {
        // 1. 消息插入时更新会话最后消息和未读数
        R"(
            CREATE TRIGGER IF NOT EXISTS trigger_conversation_insert
            AFTER INSERT ON messages
            FOR EACH ROW
            BEGIN
                UPDATE conversations
                SET last_message_content = NEW.content,
                    last_message_time = NEW.msg_time,
                    unread_count = unread_count + 1
                WHERE conversation_id = NEW.conversation_id;
            END
        )",

        // 2. 消息删除时更新会话最后消息（仅在删除的是最后一条时）
        R"(
            CREATE TRIGGER IF NOT EXISTS trigger_conversation_delete
            AFTER DELETE ON messages
            FOR EACH ROW
            BEGIN
                UPDATE conversations
                SET last_message_content = (
                    SELECT content FROM messages
                    WHERE conversation_id = OLD.conversation_id
                    ORDER BY msg_time DESC, message_id DESC
                    LIMIT 1
                ),
                last_message_time = (
                    SELECT msg_time FROM messages
                    WHERE conversation_id = OLD.conversation_id
                    ORDER BY msg_time DESC, message_id DESC
                    LIMIT 1
                )
                WHERE conversation_id = OLD.conversation_id
                AND last_message_time <= OLD.msg_time;
            END
        )",

        // 3. 插入新用户且 is_current=1 时，确保其他用户 is_current=0
        R"(
            CREATE TRIGGER IF NOT EXISTS trigger_unique_current_user_insert
            BEFORE INSERT ON users
            WHEN NEW.is_current = 1
            BEGIN
                UPDATE users SET is_current = 0;
            END
        )",

        // 4. 更新用户且 is_current 被设为 1 时，确保其他用户 is_current=0
        R"(
            CREATE TRIGGER IF NOT EXISTS trigger_unique_current_user_update
            BEFORE UPDATE ON users
            WHEN NEW.is_current = 1
            BEGIN
                UPDATE users SET is_current = 0 WHERE user_id != NEW.user_id;
            END
        )",

        // 5. 用户头像变更时更新对应单聊会话的头像
        R"(
            CREATE TRIGGER IF NOT EXISTS trigger_update_conversation_avatar_on_user_update
            AFTER UPDATE OF avatar, avatar_local_path ON users
            FOR EACH ROW
            WHEN OLD.avatar IS NOT NULL OR OLD.avatar_local_path IS NOT NULL
            BEGIN
                UPDATE conversations
                SET avatar = NEW.avatar,
                    avatar_local_path = NEW.avatar_local_path
                WHERE user_id = NEW.user_id AND type = 0;
            END
        )",

        // 6. 联系人备注名变更时更新对应单聊会话的标题
        R"(
            CREATE TRIGGER IF NOT EXISTS trigger_update_conversation_title_on_contact_update
            AFTER UPDATE OF remark_name ON contacts
            FOR EACH ROW
            WHEN OLD.remark_name IS NOT NULL
            BEGIN
                UPDATE conversations
                SET title =
                    CASE
                        WHEN NEW.remark_name IS NOT NULL AND NEW.remark_name != ''
                        THEN NEW.remark_name
                        ELSE (SELECT nickname FROM users WHERE user_id = NEW.user_id)
                    END
                WHERE user_id = NEW.user_id AND type = 0;
            END
        )",

        // 7. 群组信息变更时更新对应群聊会话的标题和头像
        R"(
            CREATE TRIGGER IF NOT EXISTS trigger_update_conversation_on_group_update
            AFTER UPDATE OF group_name, avatar, avatar_local_path ON groups
            FOR EACH ROW
            WHEN OLD.group_name IS NOT NULL OR OLD.avatar IS NOT NULL OR OLD.avatar_local_path IS NOT NULL
            BEGIN
                UPDATE conversations
                SET title = NEW.group_name,
                    avatar = NEW.avatar,
                    avatar_local_path = NEW.avatar_local_path
                WHERE group_id = NEW.group_id AND type = 1;
            END
        )"
    };
}

/**
 * @brief 获取创建数据库索引的SQL语句
 */
QString DatabaseSchema::getCreateIndexes() {
    return R"(
        -- 用户表索引
        CREATE UNIQUE INDEX IF NOT EXISTS idx_users_current ON users(is_current) WHERE is_current = 1;
        CREATE INDEX IF NOT EXISTS idx_users_account ON users(account);

        -- 联系人表索引
        CREATE INDEX IF NOT EXISTS idx_contacts_starred ON contacts(is_starred);
        CREATE INDEX IF NOT EXISTS idx_contacts_blocked ON contacts(is_blocked);
        CREATE INDEX IF NOT EXISTS idx_contacts_add_time ON contacts(add_time);

        -- 群组相关索引
        CREATE INDEX IF NOT EXISTS idx_group_members_user ON group_members(user_id);
        CREATE INDEX IF NOT EXISTS idx_group_members_group_role ON group_members(group_id, role);
        CREATE INDEX IF NOT EXISTS idx_groups_name ON groups(group_name);

        -- 会话表索引
        CREATE INDEX IF NOT EXISTS idx_conversations_last_time ON conversations(last_message_time DESC);
        CREATE INDEX IF NOT EXISTS idx_conversations_top_time ON conversations(is_top, last_message_time DESC);
        CREATE INDEX IF NOT EXISTS idx_conversations_group_user ON conversations(group_id, user_id);
        CREATE INDEX IF NOT EXISTS idx_conversations_type ON conversations(type);

        -- 消息表索引
        CREATE INDEX IF NOT EXISTS idx_messages_conversation_time ON messages(conversation_id, msg_time DESC);
        CREATE INDEX IF NOT EXISTS idx_messages_sender_time ON messages(sender_id, msg_time DESC);
        CREATE INDEX IF NOT EXISTS idx_messages_time ON messages(msg_time DESC);
        CREATE INDEX IF NOT EXISTS idx_messages_type ON messages(type);

        -- 媒体缓存索引
        CREATE INDEX IF NOT EXISTS idx_media_url ON media_cache(original_url);
        CREATE INDEX IF NOT EXISTS idx_media_access ON media_cache(last_access_time DESC);
        CREATE INDEX IF NOT EXISTS idx_media_type ON media_cache(file_type);

        -- 朋友圈表索引（全部移到这里）
        CREATE INDEX IF NOT EXISTS idx_local_moment_user ON local_moment(user_id);
        CREATE INDEX IF NOT EXISTS idx_local_moment_create_time ON local_moment(create_time DESC);
        CREATE INDEX IF NOT EXISTS idx_local_moment_sync_status ON local_moment(sync_status);
        CREATE INDEX IF NOT EXISTS idx_local_moment_expire ON local_moment(expire_time);
        CREATE INDEX IF NOT EXISTS idx_local_moment_deleted ON local_moment(is_deleted);
        CREATE INDEX IF NOT EXISTS idx_local_moment_interact_moment ON local_moment_interact(moment_id);
        CREATE INDEX IF NOT EXISTS idx_local_moment_interact_time ON local_moment_interact(local_update_time DESC);

        -- 好友申请表索引
        CREATE INDEX IF NOT EXISTS idx_friend_requests_status ON friend_requests(status);
        CREATE INDEX IF NOT EXISTS idx_friend_requests_is_read ON friend_requests(is_read);
        CREATE INDEX IF NOT EXISTS idx_friend_requests_created ON friend_requests(created_at DESC);
        CREATE INDEX IF NOT EXISTS idx_friend_requests_from_user ON friend_requests(from_user_id);
)";
}
