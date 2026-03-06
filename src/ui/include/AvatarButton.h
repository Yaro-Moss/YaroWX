#ifndef AVATARBUTTON_H
#define AVATARBUTTON_H

#include "Contact.h"
#include <QAbstractButton>
#include <QPixmap>
#include <QColor>
#include <QFont>

class WeChatWidget;
class MediaDialog;

class AvatarButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(int radius READ radius WRITE setRadius)

public:
    // 显示模式枚举
    enum Mode {
        AvatarMode,  // 头像模式（默认）
        NameMode     // 名称模式（仅显示文本）
    };
    Q_ENUM(Mode)

    explicit AvatarButton(QWidget *parent = nullptr);

    // 设置联系人/用户数据
    void setContact(const Contact &contact);
    void setUser(const User &user);

    // 圆角半径
    void setRadius(int radius);
    int radius() const { return m_radius; }

    // 设置关联窗口
    void setWeChatWidget(WeChatWidget* weChatWidget) { m_weChatWidget = weChatWidget; }
    void setMediaDialog(MediaDialog* mediaDialog) { m_mediaDialog = mediaDialog; }

    // 模式相关
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    // 自定义名称模式的字体和颜色（若不调用则使用默认样式）
    void setNameModeFont(const QFont &font);
    void setNameModeColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;  // 重写以便名称模式自适应文本宽度

private:
    // 辅助函数：生成带首字母的默认头像（严格匹配按钮圆角）
    QPixmap generateDefaultAvatar(const QSize& size, const QString& nickname);
    // 辅助函数：根据昵称生成个性化背景色
    QColor generateBgColorFromNickname(const QString& nickname);
    // 辅助函数：获取当前有效的昵称（优先Contact，其次User）
    QString getValidNickname() const;
    // 辅助函数：获取名称模式下要显示的文本（优先 remarkName，其次 nickname）
    QString getNameModeText() const;

    Contact m_contact;
    User m_user;
    int m_radius = 6;

    // 模式相关成员
    Mode m_mode = AvatarMode;
    QFont m_nameFont = QFont("微软雅黑", 13);
    QColor m_nameColor = QColor(31, 158, 211);  // rgb(31,158,211)

    WeChatWidget *m_weChatWidget;
    MediaDialog *m_mediaDialog;
};

#endif // AVATARBUTTON_H
