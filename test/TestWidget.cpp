#include "TestWidget.h"
#include "ORM.h"
#include "ui_TestWidget.h"
#include "GenerationWorker.h"
#include "AppController.h"

#include <QDebug>
#include <QMessageBox>
#include <QMovie>
#include <QLabel>
#include <QVBoxLayout>

TestWidget::TestWidget(AppController * appController, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TestWidget)
{
    ui->setupUi(this);
    m_messageController = appController->messageController();

    // 创建初始工作线程
    createWorkerThread();

    connect(m_messageController, &MessageController::send, m_worker, &GenerationWorker::sendMsg);
    connect(m_worker, &GenerationWorker::reaction, this, [this](const Message &msg){
        QtConcurrent::run([msg]() {
            Orm orm;
            Message copy = msg;
            if (orm.insert(copy)) {
                qDebug() << "Message saved to DB, id:" << copy.message_id();
            } else {
                qWarning() << "Failed to save message to DB, id:" << msg.message_id();
            }
        });
    });

}

TestWidget::~TestWidget()
{
    // 停止工作线程
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }

    delete ui;
}

void TestWidget::createWorkerThread()
{
    // 如果已有线程，先清理
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        delete m_worker;
    }

    // 创建新的工作线程
    m_workerThread = new QThread(this);
    m_worker = new GenerationWorker();

    // 将工作者移动到工作线程
    m_worker->moveToThread(m_workerThread);

    // 连接信号槽
    connect(m_worker, &GenerationWorker::progressChanged,
            this, &TestWidget::onProgressChanged);
    connect(m_worker, &GenerationWorker::nonFriendsGenerated,
            this, &TestWidget::onNonFriendsGenerated);
    connect(m_worker, &GenerationWorker::friendsGenerated,
            this, &TestWidget::onFriendsGenerated);
    connect(m_worker, &GenerationWorker::errorOccurred,
            this, &TestWidget::onErrorOccurred);

    connect(m_workerThread, &QThread::finished,
            m_worker, &GenerationWorker::deleteLater);
    connect(m_workerThread, &QThread::finished,
            this, &TestWidget::onWorkerFinished);

    // 启动工作线程
    m_workerThread->start();
}

void TestWidget::setGeneratingState(bool generating)
{
    m_isGenerating = generating;

    // 禁用/启用按钮
    ui->pushButton1->setEnabled(!generating);
    ui->pushButton_2->setEnabled(!generating);
    ui->pushButton_3->setEnabled(!generating);
    ui->spinBox->setEnabled(!generating);
    ui->spinBox_2->setEnabled(!generating);
    ui->lineEdit->setEnabled(!generating);

    if (generating) {
        qDebug() << "生成操作开始，UI已禁用";
    } else {
        qDebug() << "生成操作结束，UI已启用";
    }
}

void TestWidget::on_pushButton1_clicked()
{
    if (m_isGenerating) {
        QMessageBox::warning(this, "警告", "当前有生成任务正在运行，请等待完成");
        return;
    }

    int userCount = ui->spinBox->value();

    if (userCount <= 0) {
        QMessageBox::warning(this, "输入错误", "请输入大于0的用户数量");
        return;
    }

    if (userCount > 1000) {
        QMessageBox::warning(this, "数量过大", "建议每次生成不超过1000个用户");
        return;
    }

    // 设置生成状态
    setGeneratingState(true);

    // 在工作线程中执行生成任务
    QMetaObject::invokeMethod(m_worker, "generateNonFriends",
                              Q_ARG(int, userCount));
}

void TestWidget::on_pushButton_2_clicked()
{
    if (m_isGenerating) {
        QMessageBox::warning(this, "警告", "当前有生成任务正在运行，请等待完成");
        return;
    }

    int friendCount = ui->spinBox_2->value();

    if (friendCount <= 0) {
        QMessageBox::warning(this, "输入错误", "请输入大于0的好友数量");
        return;
    }

    if (friendCount > 500) {
        QMessageBox::warning(this, "数量过大", "建议每次生成不超过500个好友");
        return;
    }

    // 设置生成状态
    setGeneratingState(true);

    // 在工作线程中执行生成任务
    QMetaObject::invokeMethod(m_worker, "generateFriends",
                              Q_ARG(int, friendCount));
}

void TestWidget::on_pushButton_3_clicked()
{
    if (m_isGenerating) {
        QMessageBox::warning(this, "警告", "当前有生成任务正在运行，请等待完成");
        return;
    }

    QString apiKey = ui->lineEdit->text().trimmed();
    QString id = ui->lineEdit_2->text().trimmed();

    if (apiKey.isEmpty() || id.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "API Key/id不能为空");
        return;
    }

    QMetaObject::invokeMethod(m_worker, "setKey",
                              Q_ARG(QString, apiKey));
    QMetaObject::invokeMethod(m_worker, "setId",
                              Q_ARG(QString, id));
    ui->label_6->setText("当前key："+apiKey);

}

void TestWidget::onProgressChanged(int current, int total, const QString& message, int type)
{
    qDebug() << "进度:" << current << "/" << total << message;

    // 更新UI进度条
    if(type==0){
        ui->progressBar_1->setMaximum(total);
        ui->progressBar_1->setValue(current);
    }else {
        ui->progressBar_2->setMaximum(total);
        ui->progressBar_2->setValue(current);
    }
}

void TestWidget::onNonFriendsGenerated(bool success, const QString& message)
{
    setGeneratingState(false);

    if (success) {
        QMessageBox::information(this, "完成", message);
    } else {
        QMessageBox::warning(this, "生成失败", message);
    }

    qDebug() << "非好友用户生成完成:" << message;
    ui->progressBar_1->setMaximum(100);
    ui->progressBar_1->setValue(0);

}

void TestWidget::onFriendsGenerated(bool success, const QString& message)
{
    setGeneratingState(false);

    if (success) {
        QMessageBox::information(this, "完成", message);
    } else {
        QMessageBox::warning(this, "生成失败", message);
    }

    qDebug() << "好友生成完成:" << message;
    ui->progressBar_2->setMaximum(100);
    ui->progressBar_2->setValue(0);

}

void TestWidget::onErrorOccurred(const QString& error)
{
    setGeneratingState(false);

    QMessageBox::critical(this, "错误", error);
    qDebug() << "工作线程错误:" << error;

    // 重新创建工作线程
    createWorkerThread();
}

void TestWidget::onWorkerFinished()
{
    qDebug() << "工作线程结束";
    m_workerThread = nullptr;
    m_worker = nullptr;
}



