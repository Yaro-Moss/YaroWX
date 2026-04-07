#include "ContactItemDelegate.h"
#include "Contact.h"
#include "ThumbnailResourceManager.h"
#include <qabstractitemview.h>
#include <qpainterpath.h>

ContactItemDelegate::ContactItemDelegate(ContactTreeModel *model, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_model(model)
{
    ThumbnailResourceManager* thumbnailManager = ThumbnailResourceManager::instance();
    connect(thumbnailManager, &ThumbnailResourceManager::mediaLoaded,
            this, &ContactItemDelegate::onMediaLoaded);
}

void ContactItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    if (!index.isValid() || !m_model) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRect rect = option.rect;

    // ========== 1. 父节点（分组标题） ==========
    if (m_model->isParentNode(index)) {
        QString text = index.data(Qt::DisplayRole).toString();

        // 父节点背景保持原样（可选，也可以交给视图，但通常分组标题不需要悬停/选中）
        painter->fillRect(rect, QColor(247, 247, 247));

        QRect textRect(rect.left() + 12, rect.top(),
                       rect.width() - 24, rect.height());

        painter->setPen(QColor(0, 0, 0));
        QFont font = option.font;
        font.setPointSizeF(11);
        font.setBold(false);
        font.setFamily(QStringLiteral("微软雅黑"));
        painter->setFont(font);

        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

        painter->restore();
        return;
    }

    // ========== 2. 好友请求节点 ==========
    if (m_model->isFriendRequestNode(index)) {
        FriendRequest req = m_model->getFriendRequestByIndex(index);
        if (!req.isValid()) {
            painter->restore();
            return;
        }

        // 移除所有选中/悬停背景绘制，背景完全透明（由视图负责整行背景）
        const int avatarSize = 40;
        const int margin = 12;
        QRect avatarRect(rect.left() + margin, rect.center().y() - avatarSize / 2,
                         avatarSize, avatarSize);

        // 绘制头像
        ThumbnailResourceManager *mediaManager = ThumbnailResourceManager::instance();
        QPixmap avatar = mediaManager->getThumbnail(
            req.from_avatarValue(),
            QSize(avatarSize, avatarSize),
            MediaType::Avatar,
            5,
            "",
            req.from_nicknameValue()
            );
        if (!avatar.isNull()) {
            painter->drawPixmap(avatarRect, avatar);
        } else {
            drawDefaultAvatar(painter, avatarRect, req.from_nicknameValue(), 5);
        }

        // 主文本：昵称或账号
        QString mainText = req.from_nicknameValue();
        if (mainText.isEmpty()) mainText = req.from_accountValue();
        QRect textRect(avatarRect.right() + margin, avatarRect.top(),
                       rect.width() - avatarSize - margin * 3, 22);

        QFont font = option.font;
        font.setPointSizeF(11);
        font.setFamily(QStringLiteral("微软雅黑"));
        painter->setFont(font);
        painter->setPen(QColor(0, 0, 0));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, mainText);

        // 附言（较小字体，灰色）
        if (!req.messageValue().isEmpty()) {
            QRect msgRect(avatarRect.right() + margin, avatarRect.top() + 22,
                          rect.width() - avatarSize - margin * 3, 20);
            QFont smallFont = font;
            smallFont.setPointSizeF(9);
            painter->setFont(smallFont);
            painter->setPen(QColor(120, 120, 120));
            painter->drawText(msgRect, Qt::AlignLeft | Qt::AlignTop, req.messageValue());
        }

        // 右侧状态显示（右上角）
        QRect statusRect(rect.right() - 60, rect.top() + 8, 50, 24);
        if (req.statusValue() == 0) { // 待处理
            painter->setPen(QColor(100, 100, 100));
            painter->drawText(statusRect, Qt::AlignCenter, QStringLiteral("待处理"));
        } else if (req.statusValue() == 1) {
            painter->setPen(QColor(100, 100, 100));
            painter->drawText(statusRect, Qt::AlignCenter, QStringLiteral("已同意"));
        } else {
            painter->setPen(QColor(150, 150, 150));
            painter->drawText(statusRect, Qt::AlignCenter, QStringLiteral("已拒绝"));
        }

        painter->restore();
        return;
    }

    // ========== 3. 普通联系人节点 ==========
    Contact contact = index.data(Qt::UserRole).value<Contact>();

    const int avatarSize = 40;
    const int margin = 12;
    QRect avatarRect(rect.left() + margin, rect.center().y() - avatarSize / 2,
                     avatarSize, avatarSize);

    // 绘制头像
    ThumbnailResourceManager *mediaManager = ThumbnailResourceManager::instance();
    QPixmap avatar = mediaManager->getThumbnail(
        contact.user.avatar_local_pathValue(),
        QSize(avatarSize, avatarSize),
        MediaType::Avatar,
        5,
        "",
        contact.user.nicknameValue()
        );
    if (!avatar.isNull()) {
        painter->drawPixmap(avatarRect, avatar);
    } else {
        drawDefaultAvatar(painter, avatarRect, contact.user.nicknameValue(), 5);
    }

    // 显示文本（备注名 > 昵称）
    QString displayName = contact.remark_nameValue();
    if (displayName.isEmpty()) displayName = contact.user.nicknameValue();
    QRect textRect(avatarRect.right() + margin, avatarRect.top(),
                   rect.width() - avatarSize - margin * 3, avatarSize);

    painter->setPen(QColor(0, 0, 0));
    QFont font = option.font;
    font.setPointSizeF(11);
    font.setFamily(QStringLiteral("微软雅黑"));
    painter->setFont(font);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, displayName);

    painter->restore();
}

QSize ContactItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (m_model->isParentNode(index))
        return QSize(option.rect.width(), 36);
    return QSize(option.rect.width(), 60);
}

void ContactItemDelegate::drawDefaultAvatar(QPainter *painter, const QRect &avatarRect,
                                            const QString &name, int radius) const
{
    QPainterPath path;
    path.addRoundedRect(QRectF(avatarRect), radius, radius);
    painter->setPen(Qt::NoPen);

    // 使用更柔和的颜色
    QLinearGradient gradient(avatarRect.topLeft(), avatarRect.bottomRight());
    gradient.setColorAt(0, QColor(180, 180, 180));
    gradient.setColorAt(1, QColor(140, 140, 140));

    painter->setBrush(gradient);
    painter->drawPath(path);

    QFont iniFont = painter->font();
    iniFont.setBold(true);
    iniFont.setPointSize(15);
    painter->setFont(iniFont);
    painter->setPen(QColor(255, 255, 255));  // 白色文字，对比度更好
    painter->drawText(avatarRect, Qt::AlignCenter, name.isEmpty() ? "?" : name.left(1).toUpper());
}


void ContactItemDelegate::onMediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type)
{
    if(type == MediaType::Avatar) {
        // 通知视图更新
        if(QAbstractItemView* view = qobject_cast<QAbstractItemView*>(parent())) {
            view->viewport()->update();
        }
    }
}
