#include "AvatarButton.h"
#include "CurrentUserInfoDialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QDebug>

AvatarButton::AvatarButton(QWidget *parent)
    : QAbstractButton(parent)
    , m_weChatWidget(nullptr)
    , m_mediaDialog(nullptr)
{
    setCheckable(false);
    setMinimumSize(24, 24);
    setContentsMargins(0, 0, 0, 0);

    connect(this, &AvatarButton::clicked, this, [this](){
        CurrentUserInfoDialog *currentUserInfoDialog = new CurrentUserInfoDialog(this);
        currentUserInfoDialog->setAttribute(Qt::WA_DeleteOnClose);
        currentUserInfoDialog->setCurrentUser(m_contact);
        QPoint btnGlobalPos = this->mapToGlobal(QPoint(0,0));
        currentUserInfoDialog->showAtPos(QPoint(btnGlobalPos.x() + width(), btnGlobalPos.y()));
        currentUserInfoDialog->setWeChatWidget(m_weChatWidget);
        currentUserInfoDialog->setMediaDialog(m_mediaDialog);
    });
}

void AvatarButton::setContact(const Contact &contact)
{
    m_contact = contact;
    m_user = User();
    update();
    updateGeometry();
}

void AvatarButton::setUser(const User &user)
{
    m_user = user;
    m_contact = Contact();
    update();
    updateGeometry();
}

void AvatarButton::setRadius(int radius)
{
    if (m_radius != radius) {
        m_radius = radius;
        update();
    }
}

// 模式设置
void AvatarButton::setMode(Mode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        update();          // 立即重绘
        updateGeometry();  // 通知布局尺寸可能变化
    }
}

// 名称模式字体设置
void AvatarButton::setNameModeFont(const QFont &font)
{
    m_nameFont = font;
    if (m_mode == NameMode) {
        update();
        updateGeometry();
    }
}

// 名称模式颜色设置
void AvatarButton::setNameModeColor(const QColor &color)
{
    m_nameColor = color;
    if (m_mode == NameMode) {
        update();
    }
}

// 获取名称模式显示的文本
QString AvatarButton::getNameModeText() const
{
    // 优先使用联系人的 remarkName
    if (m_contact.isValid() && !m_contact.remarkName.isEmpty()) {
        return m_contact.remarkName;
    }
    // 其次使用联系人昵称
    if (m_contact.isValid() && !m_contact.user.nickname.isEmpty()) {
        return m_contact.user.nickname;
    }
    // 再其次使用单独设置的 User 昵称
    if (!m_user.nickname.isEmpty()) {
        return m_user.nickname;
    }
    // 最后返回空字符串，由调用者决定显示什么（paintEvent中会用占位符）
    return QString();
}

// 重写 paintEvent
void AvatarButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // ========== 名称模式：只绘制文本，背景完全透明 ==========
    if (m_mode == NameMode) {
        // 不绘制任何背景，保持透明，由父窗口透出
        painter.setPen(m_nameColor);
        painter.setFont(m_nameFont);

        QString text = getNameModeText();
        if (text.isEmpty()) {
            text = "?";  // 兜底字符，可自定义
        }

        // 绘制文本：左对齐，垂直居中，不换行
        QRect textRect = rect();
        // 添加左边距 4px，使文字不紧贴边缘
        textRect.adjust(4, 0, 0, 0);

        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
        return; // 名称模式处理完毕，直接返回
    }

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRect drawRect = this->rect();
    if (drawRect.isEmpty()) return;

    QPixmap avatarPixmap;
    // 加载头像图片
    if (m_contact.isValid()) {
        avatarPixmap = QPixmap(m_contact.user.avatarLocalPath);
    } else if (!m_user.avatarLocalPath.isEmpty()) {
        avatarPixmap = QPixmap(m_user.avatarLocalPath);
    }

    // 图片加载失败则生成默认头像（圆角严格匹配m_radius）
    if (avatarPixmap.isNull()) {
        avatarPixmap = generateDefaultAvatar(drawRect.size(), getValidNickname());
    }

    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(drawRect), m_radius, m_radius);
    painter.setClipPath(clipPath);

    QPixmap scaledPix = avatarPixmap.scaled(
        drawRect.size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
        );

    // 计算居中偏移，确保图片居中且完全覆盖按钮
    int x = drawRect.x() - (scaledPix.width() - drawRect.width()) / 2;
    int y = drawRect.y() - (scaledPix.height() - drawRect.height()) / 2;

    painter.drawPixmap(x, y, scaledPix);
}

// 重写 sizeHint，使名称模式下按钮宽度根据文本自动调整
QSize AvatarButton::sizeHint() const
{
    if (m_mode == NameMode) {
        QFontMetrics fm(m_nameFont);
        QString text = getNameModeText();
        if (text.isEmpty()) text = "?";
        int width = fm.horizontalAdvance(text) + 8;  // 左右边距各4px
        int height = fm.height() + 4;                 // 上下边距各2px
        return QSize(width, height).expandedTo(QAbstractButton::sizeHint());
    }
    return QAbstractButton::sizeHint();
}


QPixmap AvatarButton::generateDefaultAvatar(const QSize& size, const QString& nickname)
{
    QPixmap defaultAvatar(size);
    defaultAvatar.fill(Qt::transparent);

    QPainter painter(&defaultAvatar);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, size.width(), size.height()), m_radius, m_radius);
    painter.setPen(Qt::NoPen);
    painter.setBrush(generateBgColorFromNickname(nickname));
    painter.drawPath(path);

    // 绘制首字母（居中显示）
    QString displayText = nickname.isEmpty() ? "U" : nickname.left(1).toUpper();

    QFont font = painter.font();
    font.setBold(true);
    int fontSize = qMin(size.width(), size.height()) * 0.5;
    fontSize = qMax(fontSize, 12);
    font.setPixelSize(fontSize);
    painter.setFont(font);

    painter.setPen(Qt::white);
    painter.drawText(QRect(0, 0, size.width(), size.height()),
                     Qt::AlignCenter | Qt::TextSingleLine,
                     displayText);

    return defaultAvatar;
}

QColor AvatarButton::generateBgColorFromNickname(const QString& nickname)
{
    if (nickname.isEmpty()) {
        return QColor(210, 210, 210);
    }
    uint hash = qHash(nickname);
    int hue = hash % 360;
    int saturation = 80;
    int lightness = 70;
    return QColor::fromHsl(hue, saturation, lightness);
}

QString AvatarButton::getValidNickname() const
{
    if (m_contact.isValid() && !m_contact.user.nickname.isEmpty()) {
        return m_contact.user.nickname;
    }
    if (!m_user.nickname.isEmpty()) {
        return m_user.nickname;
    }
    return "";
}
