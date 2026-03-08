#include "DoubaoAi.h"


DoubaoAI::DoubaoAI(): OPENAI_API_KEY(QString()), OPENAI_MODEL(QString()){};

DoubaoAI::DoubaoAI(QString api_key, QString id):OPENAI_API_KEY(api_key), OPENAI_MODEL(id){}

QString DoubaoAI::DoubaoAI_request(QString& question)
{
    QNetworkRequest request;
    QNetworkAccessManager manager;
    request.setUrl(QUrl(OPENAI_BASE_URL));
    
    //头部信息设置
    QString token = "Bearer " + OPENAI_API_KEY;
    request.setRawHeader("Authorization", token.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //post附加信息,用json格式构建官方示例里面的请求体内容
    QJsonArray context;
    QJsonObject textobj;
    textobj["type"] = "text";
    textobj["text"] = question;
    context.append(textobj);

    QJsonArray messagesarray;
    QJsonObject messageobj;
    messageobj["role"] = "user";
    messageobj["content"] = context;
    messagesarray.append(messageobj);

    QJsonObject data;
    data["model"] = OPENAI_MODEL;
    data["messages"] = messagesarray;

    QJsonDocument doc(data);
    QByteArray postData = doc.toJson();

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::AnyProtocol);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(config);

    //发送post请求
    QNetworkReply *reply = manager.post(request, postData);

    //循环等待服务器返回数据
    QEventLoop waitserver;
    connect(reply, &QNetworkReply::finished, &waitserver, &QEventLoop::quit);
    waitserver.exec();
    
    if(reply != nullptr && reply->error() == QNetworkReply::NoError){
    	//得到返回的数据
        QByteArray reply_data = reply->readAll();
        //对返回的json数据进行解析
        QJsonObject obj = QJsonDocument::fromJson(reply_data).object();
        QJsonArray choicesarray = obj.value("choices").toArray();
        if (!choicesarray.isEmpty()){
            QJsonObject choiceobj = choicesarray[0].toObject();
            if (choiceobj.contains("message") && choiceobj["message"].isObject()) {
                QJsonObject messageobj = choiceobj["message"].toObject();
                if (messageobj.contains("content") && messageobj["content"].isString()) {
                    return messageobj["content"].toString();
                }
            }
        }
        return "ai返回数据为空！";
    } else {
        return "忙碌着，有空会回复你。";
        qDebug() << "https request error:" << reply->errorString();
    }
}

