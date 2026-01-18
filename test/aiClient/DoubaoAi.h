#ifndef DOUBAOAI_H
#define DOUBAOAI_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QEventLoop>

class DoubaoAI : public QObject
{
    Q_OBJECT
public:
    DoubaoAI();
    DoubaoAI(QString api_key, QString id);
    QString DoubaoAI_request(QString& question);
    void setKey(const QString  &key){OPENAI_API_KEY = key;}
    void setID(const QString &id){OPENAI_MODEL = id ;}
private:
    QString OPENAI_BASE_URL = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";
    QString OPENAI_API_KEY;
    QString OPENAI_MODEL;
};

#endif // DOUBAOAI_H
