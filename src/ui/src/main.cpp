#include "AppInitialize.h"
#include "DatabaseInitializationController.h"
#include "WeChatWidget.h"
#include <QApplication>
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
#include "network.h"
#include "WebSocketManager.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("AAA");
    QCoreApplication::setApplicationName("YaroWX");

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

    auto network = std::make_unique<Network>();
    LoginAndRegisterDialog loginAndRegisterDialog(network->loginManager());
    auto initController = std::make_unique<DatabaseInitializationController>(network.get());
    auto appInit = std::make_unique<AppInitialize>(initController.get());
    auto appController = std::make_unique<AppController>(network.get());


    if(loginAndRegisterDialog.exec() == QDialog::Accepted){
        qDebug()<<"登录成功";
    }else {
        return app.exec();
    }

    std::unique_ptr<WeChatWidget> wechatWidget;
    QObject::connect(appInit.get(), &AppInitialize::isInited, &app, [&](){
        wechatWidget = std::make_unique<WeChatWidget>(appController.get());
        wechatWidget->show();
        WebSocketManager::instance()->connect();

    });
    appInit->initialize();

    int result = app.exec();
    // 显式清理ThumbnailResourceManager,防止QGuiApplication 销毁后还在处理 QPixmap，造成崩溃
    ThumbnailResourceManager::cleanup();

    return result;
}
