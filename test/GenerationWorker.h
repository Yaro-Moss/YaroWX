#ifndef GENERATIONWORKER_H
#define GENERATIONWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include "DoubaoAi.h"
#include "User.h"
#include "Contact.h"
#include "Message.h"

// 前向声明
class UserTable;
class ContactTable;

class GenerationWorker : public QObject
{
    Q_OBJECT

public:
    explicit GenerationWorker(QObject *parent = nullptr);
    ~GenerationWorker();

    // 初始化数据库连接
    bool initDatabase();

    void sendMsg(QVector<Message> messages);
    QString buildPrompt(const QVector<Message> &messages);

public slots:
    // 生成非好友用户
    void generateNonFriends(int count);

    // 生成好友
    void generateFriends(int count);

    // 生成并保存当前用户。
    User generateAndSaveCurrentUser();

    void setKey(const QString key){m_key = key ;}
    void setId(const QString id){m_id = id ;}


signals:
    // 进度信号
    void progressChanged(int current, int total, const QString& message, int type);

    // 完成信号
    void nonFriendsGenerated(bool success, const QString& message);
    void friendsGenerated(bool success, const QString& message);

    // 错误信号
    void errorOccurred(const QString& error);

    // 响应消息
    void reaction(Message msg);
private:
    // 生成单个用户
    User generateSingleUser();

    // 生成联系人信息
    Contact generateContactForUser(const User& user);

    // 生成随机数据函数
    QString generateRandomNickname();
    QString generateRandomRegion();
    QString generateRandomSignature();
    QString generateRandomAvatar();
    int generateRandomGender();
    QString generateRandomRemarkName(const QString& nickname);
    QString generateRandomDescription();
    QJsonArray generateRandomTags();
    QString generateRandomPhoneNote();
    QString generateRandomEmailNote();
    QString generateRandomSource();

    // 数据库操作对象
    UserTable* m_userTable = nullptr;
    ContactTable* m_contactTable = nullptr;

    QMutex m_mutex;
    bool m_initialized = false;

    DoubaoAI *doubao;
    QString m_key;
    QString m_id;
};

#endif // GENERATIONWORKER_H
