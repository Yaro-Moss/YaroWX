#ifndef SPLASHDIALOG_H
#define SPLASHDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

class SplashDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SplashDialog(QWidget *parent = nullptr);
    void setProgress(int value, const QString& message = "");

private:
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_messageLabel;
    QProgressBar *m_progressBar;
    QLabel *m_versionLabel;
};

#endif // SPLASHDIALOG_H