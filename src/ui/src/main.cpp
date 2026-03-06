#include "AppInitialize.h"
#include "DatabaseInitializationController.h"
#include "WeChatWidget.h"
#include <QApplication>
#include "DatabaseManager.h"
#include "AppController.h"
#include "GroupMember.h"
#include "Group.h"
#include "MediaCache.h"
#include "MediaItem.h"
#include "Message.h"
#include "User.h"
#include "Contact.h"
#include "Conversation.h"
#include "ThumbnailResourceManager.h"
#include <QMessageBox>
#include "LoginAndRegisterDialog.h"
#include "LoginAndRegisterController.h"
#include "TestWidget.h"



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 注册元类型
    qRegisterMetaType<Conversation>("Conversation");
    qRegisterMetaType<QList<Conversation>>("QList<Conversation>");

    qRegisterMetaType<Contact>("Contact");
    qRegisterMetaType<QList<Contact>>("QList<Contact>");

    qRegisterMetaType<GroupMember>("GroupMember");
    qRegisterMetaType<QList<GroupMember>>("QList<GroupMember>");

    qRegisterMetaType<Group>("Group");
    qRegisterMetaType<QList<Group>>("QList<Group>");

    qRegisterMetaType<MediaCache>("MediaCache");
    qRegisterMetaType<QList<MediaCache>>("QList<MediaCache>");

    qRegisterMetaType<Message>("Message");
    qRegisterMetaType<QList<Message>>("QList<Message>");

    qRegisterMetaType<MediaItem>("MediaItem");
    qRegisterMetaType<QList<MediaItem>>("QList<MediaItem>");

    qRegisterMetaType<User>("User");
    qRegisterMetaType<QList<User>>("QList<User>");

    qRegisterMetaType<LocalMoment>("LocalMoment");
    qRegisterMetaType<QHash<qlonglong, LocalMoment>>("QHash<qlonglong, LocalMoment>");
    qRegisterMetaType<QVector<LocalMoment>>("QVector<LocalMoment>");
    qRegisterMetaType<LocalMomentInteract>("LocalMomentInteract");
    qRegisterMetaType<MomentLikeInfo>("MomentLikeInfo");
    qRegisterMetaType<MomentCommentInfo>("MomentCommentInfo");

    LoginAndRegisterController loginAndRegisterController;
    LoginAndRegisterDialog loginAndRegisterDialog(&loginAndRegisterController);
    if(loginAndRegisterDialog.exec() == QDialog::Accepted){
        qDebug()<<"登录成功-----";
    }else {
        return app.exec();
    }

    DatabaseInitializationController* initController = new DatabaseInitializationController();
    AppInitialize* appInit = new AppInitialize(initController);

    DatabaseManager* databaseManager = nullptr;
    AppController* appController = nullptr;
    WeChatWidget* wechatWidget = nullptr;

    TestWidget *testWidget;//测试

    QObject::connect(appInit, &AppInitialize::isInited, &app, [&](){
        databaseManager = new DatabaseManager();
        databaseManager->start();
        appController = new AppController(databaseManager);

        // 测试----------------------------------------
        testWidget = new TestWidget(appController);
        testWidget->show();
        // --------------------------------------------


        wechatWidget = new WeChatWidget(appController);
        wechatWidget->show();
    });

    appInit->initialize();






    int result = app.exec();
    delete wechatWidget;
    delete appController;
    delete databaseManager;
    delete appInit;
    delete initController;
    delete testWidget;

    // 显式清理ThumbnailResourceManager,防止QGuiApplication 销毁后还在处理 QPixmap，造成崩溃
    ThumbnailResourceManager::cleanup();

    return result;
}
