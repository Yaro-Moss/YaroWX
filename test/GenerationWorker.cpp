#include "GenerationWorker.h"
#include "UserTable.h"
#include "ContactTable.h"
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <QStringList>
#include <QCoreApplication>


// 常量定义
const QStringList FIRST_NAMES = {
    "张", "王", "李", "赵", "刘", "陈", "杨", "黄", "周", "吴",
    "徐", "孙", "胡", "朱", "高", "林", "何", "郭", "马", "罗"
};

const QStringList LAST_NAMES = {
    "伟", "芳", "娜", "秀英", "敏", "静", "丽", "强", "磊", "军",
    "洋", "勇", "艳", "杰", "涛", "明", "超", "秀兰", "霞", "平",
    "刚", "桂英", "飞", "鹏", "华", "红", "建", "文", "云", "兰"
};

const QStringList REGIONS = {
    "北京", "上海", "广州", "深圳", "杭州", "成都", "重庆", "武汉", "西安", "南京",
    "天津", "苏州", "郑州", "长沙", "东莞", "沈阳", "青岛", "合肥", "济南", "厦门"
};

const QStringList SIGNATURES = {
    "生活就像海洋，只有意志坚强的人才能到达彼岸",
    "今天的努力，是明天的实力",
    "不忘初心，方得始终",
    "心有猛虎，细嗅蔷薇",
    "世界那么大，我想去看看",
    "简单生活，快乐自己",
    "努力成为更好的自己",
    "静坐常思己过，闲谈莫论人非",
    "行到水穷处，坐看云起时",
    "你若盛开，蝴蝶自来"
};

const QStringList AVATARS = {
    "https://example.com/avatars/male1.png",
    "https://example.com/avatars/male2.png",
    "https://example.com/avatars/female1.png",
    "https://example.com/avatars/female2.png",
    "https://example.com/avatars/default.png"
};

const QStringList TAG_LIST = {
    "同学", "同事", "朋友", "家人", "客户", "合作伙伴", "老师", "学生",
    "游戏好友", "书友", "音乐同好", "旅行伙伴", "健身伙伴", "美食家"
};

const QStringList SOURCE_LIST = {
    "手机通讯录", "QQ好友", "微信好友", "扫一扫", "群聊", "名片分享",
    "附近的人", "摇一摇", "搜索添加", "可能认识的人"
};

GenerationWorker::GenerationWorker(QObject *parent)
    : QObject(parent)
{
    doubao = new DoubaoAI();
}

GenerationWorker::~GenerationWorker()
{
    delete m_userTable;
    delete m_contactTable;
}

void GenerationWorker::sendMsg(QVector<Message> messages){
    if(m_id.isEmpty()||m_key.isEmpty()) return;

    doubao->setID(m_id);
    doubao->setKey(m_key);

    QString reqTxt = buildPrompt(messages);
    QString content = doubao->DoubaoAI_request(reqTxt);

    Message msg = messages[0];
    msg.content = content;
    msg.type = MessageType::TEXT;
    msg.timestamp = QDateTime::currentSecsSinceEpoch()+2;//回复的消息，比我发的消息慢2秒。
    qint64 tempId = std::move(msg.senderId);
    msg.senderId = std::move(msg.consigneeId);
    msg.consigneeId = std::move(tempId);

    emit reaction(msg);
}

QString GenerationWorker::buildPrompt(const QVector<Message> &messages)
{

    QString prompt = "在聊天会话里，下面是聊天过程，聊天过程中{}里面的是特殊信息，你大概想象其内容就行,"
                  "如{图片}表示图片信息、{视频}表示视频信息、{文件}表示文件信息...;"
                  "模拟聊天，"
                  "生成回复消息，不要有多余回答，就给我聊天信息回复内容就行，纯文本内容"
                  "下面是聊天过程：";

    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        const Message& msg = *it;

        QString userinfo = QString("用户%1 ：").arg(msg.senderId);
        prompt+=userinfo;

        if (msg.type == MessageType::TEXT) {
            prompt += msg.content;
            prompt+=" 。";
        } else {
            prompt += "{";
            prompt += msg.content;
            prompt += "}";
        }
    }
    return prompt;
}


// 生成并保存当前用户
User GenerationWorker::generateAndSaveCurrentUser()
{
    // 创建当前用户
    User currentUser;

    // 使用时间戳生成唯一ID和账号
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    currentUser.userId = timestamp;
    currentUser.account = QString("user_%1").arg(timestamp);
    currentUser.nickname = QString("用户%1").arg(timestamp % 10000);
    currentUser.avatar = "";
    currentUser.avatarLocalPath = "D:wallpaper/image_1.png";
    currentUser.gender = 0;
    currentUser.region = "中国";
    currentUser.signature = "欢迎使用本系统";
    currentUser.isCurrent = true;

    // 同步保存到数据库
    if (m_userTable) {
        bool saved = m_userTable->saveCurrentUser(0, currentUser);
        if (!saved) {
            qWarning() << "Failed to save current user";
            return User(); // 返回空用户
        }
    }

    Contact contact;
    contact.userId=currentUser.userId;
    contact.user = currentUser;
    contact.remarkName = currentUser.nickname;
    if(m_contactTable){
        m_contactTable->saveContact(0,contact);
    }

    return currentUser;
}

bool GenerationWorker::initDatabase()
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return true;
    }

    try {
        // 在 worker 线程中创建数据库操作对象
        m_userTable = new UserTable();
        m_contactTable = new ContactTable();

        // 调用初始化方法（假设这些类有init方法）
        bool userInit = m_userTable->init();
        bool contactInit = m_contactTable->init();

        m_initialized = userInit && contactInit;

        if (!m_initialized) {
            emit errorOccurred("数据库初始化失败");
        }

        generateAndSaveCurrentUser();
        return m_initialized;
    }
    catch (const std::exception& e) {
        emit errorOccurred(QString("数据库初始化异常: %1").arg(e.what()));
        return false;
    }
}

void GenerationWorker::generateNonFriends(int count)
{
    if (!initDatabase()) {
        emit nonFriendsGenerated(false, "数据库初始化失败");
        return;
    }

    if (count <= 0) {
        emit nonFriendsGenerated(false, "请输入有效的用户数量");
        return;
    }

    qDebug() << "在后台线程中开始生成" << count << "个非好友用户...";

    int successCount = 0;

    for (int i = 0; i < count; ++i) {
        // 生成用户
        User user = generateSingleUser();

        // 保存到数据库
        m_userTable->saveUser(0, user);

        // 发射进度信号
        emit progressChanged(i + 1, count, QString("正在生成用户: %1").arg(user.nickname), 0);

        // 让出CPU，避免长时间占用
        if (i % 10 == 0) {
            QThread::msleep(1);
            QCoreApplication::processEvents();
        }

        successCount++;
    }

    QString message = QString("生成完成。成功: %1, 失败: %2").arg(successCount).arg(count-successCount);
    qDebug() << message;
    emit nonFriendsGenerated(successCount > 0, message);
}

void GenerationWorker::generateFriends(int count)
{
    if (!initDatabase()) {
        emit friendsGenerated(false, "数据库初始化失败");
        return;
    }

    if (count <= 0) {
        emit friendsGenerated(false, "请输入有效的好友数量");
        return;
    }

    qDebug() << "在后台线程中开始生成" << count << "个好友...";

    int successCount = 0;

    for (int i = 0; i < count; ++i) {
        // 生成用户
        User user = generateSingleUser();

        // 保存用户到数据库
        m_userTable->saveUser(0, user);

        // 生成联系人
        Contact contact = generateContactForUser(user);

        // 保存联系人到数据库
        m_contactTable->saveContact(0, contact);

        // 发射进度信号
        emit progressChanged(i + 1, count, QString("正在生成好友: %1").arg(user.nickname), 1);

        // 让出CPU
        if (i % 10 == 0) {
            QThread::msleep(1);
            QCoreApplication::processEvents();
        }

        successCount++;
    }

    QString message = QString("好友生成完成。成功: %1, 失败: %2").arg(successCount).arg(count-successCount);
    qDebug() << message;
    emit friendsGenerated(successCount > 0, message);
}


// 生成随机数据
User GenerationWorker::generateSingleUser()
{
    static std::atomic<qint64> counter(0);

    User user;
    QDateTime now = QDateTime::currentDateTime();
    qint64 timestamp = now.toMSecsSinceEpoch();

    // 生成用户ID：时间戳 + 计数器
    user.userId = timestamp + counter.fetch_add(1);

    // 生成账号
    user.account = QString("user_%1").arg(now.toSecsSinceEpoch() + counter.load());

    // 生成昵称
    user.nickname = generateRandomNickname();

    // 生成头像URL
    user.avatar = generateRandomAvatar();

    // 生成本地头像路径
    user.avatarLocalPath = QString("/avatars/%1.jpg").arg(user.userId);

    // 生成性别
    user.gender = generateRandomGender();

    // 生成地区
    user.region = generateRandomRegion();

    // 生成个性签名
    user.signature = generateRandomSignature();

    // 设置是否为当前用户
    user.isCurrent = false;

    return user;
}

Contact GenerationWorker::generateContactForUser(const User& user)
{
    Contact contact;

    contact.userId = user.userId;
    contact.remarkName = generateRandomRemarkName(user.nickname);
    contact.description = generateRandomDescription();
    contact.tags = generateRandomTags();
    contact.phoneNote = generateRandomPhoneNote();
    contact.emailNote = generateRandomEmailNote();
    contact.source = generateRandomSource();
    contact.isStarred = QRandomGenerator::global()->bounded(100) < 10;
    contact.isBlocked = false;
    contact.addTime = QDateTime::currentSecsSinceEpoch();
    contact.user = user;

    return contact;
}

QString GenerationWorker::generateRandomNickname()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    QString firstName = FIRST_NAMES.at(generator->bounded(FIRST_NAMES.size()));
    QString lastName = LAST_NAMES.at(generator->bounded(LAST_NAMES.size()));

    if (generator->bounded(100) < 80) {
        return firstName + lastName;
    } else {
        QString middleName = LAST_NAMES.at(generator->bounded(LAST_NAMES.size()));
        return firstName + middleName + lastName;
    }
}

QString GenerationWorker::generateRandomRegion()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    return REGIONS.at(generator->bounded(REGIONS.size()));
}

QString GenerationWorker::generateRandomSignature()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    return SIGNATURES.at(generator->bounded(SIGNATURES.size()));
}

QString GenerationWorker::generateRandomAvatar()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    return AVATARS.at(generator->bounded(AVATARS.size()));
}

int GenerationWorker::generateRandomGender()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    int random = generator->bounded(100);

    if (random < 40) {
        return 1;
    } else if (random < 80) {
        return 2;
    } else {
        return 0;
    }
}

QString GenerationWorker::generateRandomRemarkName(const QString& nickname)
{
    QRandomGenerator *generator = QRandomGenerator::global();

    if (generator->bounded(100) < 70) {
        return nickname;
    } else {
        QStringList prefixes = {"小", "大", "阿", "老"};
        QString prefix = prefixes.at(generator->bounded(prefixes.size()));
        return prefix + nickname;
    }
}

QString GenerationWorker::generateRandomDescription()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    QStringList descriptions = {
        "大学同学", "工作同事", "高中好友", "亲戚", "客户经理",
        "合作伙伴", "兴趣小组认识的", "旅行时认识的朋友", "线上游戏队友", ""
    };

    if (generator->bounded(100) < 40) {
        return "";
    }

    return descriptions.at(generator->bounded(descriptions.size()));
}

QJsonArray GenerationWorker::generateRandomTags()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    QJsonArray tags;

    int tagCount = generator->bounded(4);

    for (int i = 0; i < tagCount; ++i) {
        QString tag = TAG_LIST.at(generator->bounded(TAG_LIST.size()));
        if (!tags.contains(tag)) {
            tags.append(tag);
        }
    }

    return tags;
}

QString GenerationWorker::generateRandomPhoneNote()
{
    QRandomGenerator *generator = QRandomGenerator::global();

    if (generator->bounded(100) < 60) {
        return "";
    }

    QString phone = "1";
    for (int i = 0; i < 10; ++i) {
        phone.append(QString::number(generator->bounded(10)));
    }

    return phone;
}

QString GenerationWorker::generateRandomEmailNote()
{
    QRandomGenerator *generator = QRandomGenerator::global();

    if (generator->bounded(100) < 70) {
        return "";
    }

    QStringList domains = {"gmail.com", "qq.com", "163.com", "126.com", "sina.com", "hotmail.com"};
    QString domain = domains.at(generator->bounded(domains.size()));
    QString name = "user" + QString::number(generator->bounded(10000));

    return name + "@" + domain;
}

QString GenerationWorker::generateRandomSource()
{
    QRandomGenerator *generator = QRandomGenerator::global();
    return SOURCE_LIST.at(generator->bounded(SOURCE_LIST.size()));
}
