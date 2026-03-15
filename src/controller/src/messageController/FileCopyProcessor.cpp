#include "FileCopyProcessor.h"
#include "ConfigManager.h"
#include "FileCopyTask.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

FileCopyProcessor::FileCopyProcessor(QObject *parent)
    : QObject(parent)
{
    // 默认最大线程数设置为CPU核心数
    int idealThreadCount = QThread::idealThreadCount();
    m_threadPool.setMaxThreadCount(qMax(2, idealThreadCount)); // 至少2个线程
}

FileCopyProcessor::~FileCopyProcessor()
{
    m_threadPool.waitForDone(5000);
}

void FileCopyProcessor::copyFile(const qint64 taskId, const QString &sourceFilePath, const QString &targetDir, int priority)
{
    copyFiles(taskId, QStringList() << sourceFilePath, targetDir, priority);
}

void FileCopyProcessor::copyFiles(const qint64 taskId, const QStringList &sourceFilePaths, const QString &targetDir, int priority)
{
    QString tDr = targetDir.isEmpty()? getDefaultSavePath():targetDir;

    foreach (const QString &filePath, sourceFilePaths) {
        // 创建复制任务
        FileCopyTask *task = new FileCopyTask(taskId, filePath, tDr, priority);

        // 连接任务完成信号
        connect(task, &FileCopyTask::finished,
                this, &FileCopyProcessor::onTaskFinished);

        // 提交到线程池
        m_threadPool.start(task);
    }
}

void FileCopyProcessor::onTaskFinished(const qint64 taskId, bool success, const QString &sourcePath,
                                       const QString &targetPath, const QString &errorMessage)
{
    emit fileCopied(taskId, success, sourcePath, targetPath, errorMessage);
}

void FileCopyProcessor::setMaxThreadCount(int count)
{
    m_threadPool.setMaxThreadCount(qMax(1, count));
}

void FileCopyProcessor::cancelAllTasks()
{
    m_threadPool.clear();
}

QString FileCopyProcessor::getDefaultSavePath()
{
    ConfigManager* configManager = ConfigManager::instance();
    QDir baseDir(configManager->dataSavePath());
    QString copyPath = baseDir.absoluteFilePath("file");

    // 确保目录存在
    baseDir.mkpath("file");

    return copyPath;
}
