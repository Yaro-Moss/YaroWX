#include "SplashDialog.h"
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QStyle>

SplashDialog::SplashDialog(QWidget *parent)
    : QDialog(parent)
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 300);
    
    // 设置样式
    setStyleSheet(R"(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2c3e50, stop:1 #3498db);
            border-radius: 10px;
            border: 2px solid #34495e;
        }
        QLabel {
            color:rgb(7, 193, 96);
            background: transparent;
        }
        QProgressBar {
            border: 2px solid #34495e;
            border-radius: 5px;
            text-align: center;
            color: white;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #27ae60, stop:1 #2ecc71);
            border-radius: 3px;
        }
    )");

    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 应用图标
    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(QPixmap(":/a/icons/app_icon.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconLabel->setAlignment(Qt::AlignCenter);
    
    // 标题
    m_titleLabel = new QLabel("数据初始化", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    
    // 消息标签
    m_messageLabel = new QLabel("正在准备应用程序...", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("%p%");
    
    // 版本信息
    m_versionLabel = new QLabel("版本 1.0.0", this);
    m_versionLabel->setAlignment(Qt::AlignCenter);
    m_versionLabel->setStyleSheet("color: #bdc3c7; font-size: 10px;");
    
    // 添加到布局
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_iconLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(m_messageLabel);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_versionLabel);
    mainLayout->addSpacing(20);
    
    // 居中显示
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    move(screenGeometry.center() - rect().center());
}

void SplashDialog::setProgress(int value, const QString& message)
{
    m_progressBar->setValue(value);
    if (!message.isEmpty()) {
        m_messageLabel->setText(message);
    }
    
    // 强制更新界面
    QApplication::processEvents();
}
