#ifndef LOGINANDREGISTERDIALOG_H
#define LOGINANDREGISTERDIALOG_H

#include <QDialog>
#include <QPoint>
#include <QPixmap>


namespace Ui { class LoginAndRegisterDialog; }
class LoginManager;

class LoginAndRegisterDialog : public QDialog
{
    Q_OBJECT

    
public:
    explicit LoginAndRegisterDialog(LoginManager *loginManager, QWidget *parent = nullptr);
    ~LoginAndRegisterDialog() override;


protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;


private slots:
    void on_closeToolButton_clicked();

    void on_switchSMSLoginBnt_clicked();
    void on_switchPasswordLoginBnt_clicked();
    void on_switchWechatNumBnt_clicked();

    void on_LoginOrRegister_clicked();

    void on_actLoginBnt_clicked();
    void on_actRegisterBnt_clicked();

    void on_phoneRegisterBnt_clicked();
    void on_phoneLoginBnt_clicked();

private:
    Ui::LoginAndRegisterDialog *ui;
    QPixmap background;
    bool m_isDragging;
    QPoint m_dragStartPosition;
    LoginManager *m_loginManager;

    void login(const QString &account, const QString& password);
    void onLoginSuccess();
    void onLoginFailed(const QString &reason);
    void onNetworkError(const QString &error);
};


#endif // LOGINANDREGISTERDIALOG_H
