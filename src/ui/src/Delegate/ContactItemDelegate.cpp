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
    if (!index.isValid())
        return;

    if (!m_model) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRect rect = option.rect;

    // 绘制背景
    if (option.state & QStyle::State_Selected) {
        // 选中状态
        painter->fillRect(rect, QColor(240, 240, 240));
    } else if (option.state & QStyle::State_MouseOver) {
        // 鼠标悬停状态 - 只在可选中项上显示
        if (!m_model->isParentNode(index)) {
            painter->fillRect(rect, QColor(240, 240, 240));
        }
    }

    if(m_model->isParentNode(index)){
        QString text = index.data(Qt::DisplayRole).toString();

        // 父节点背景
        painter->fillRect(rect, QColor(247, 247, 247));

        QRect textRect(rect.left() + 12, rect.top(),
                       rect.width() - 24, rect.height());

        painter->setPen(QColor(0, 0, 0));
        QFont font = option.font;
        font.setPointSizeF(11);
        font.setBold(false);
        font.setFamily(QStringLiteral("微软雅黑"));
        painter->setFont(font);

        // 绘制文本
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

        painter->restore();
        return;
    }

    const int avatarSize = 40;
    const int margin = 12;

    Contact contact = index.data(Qt::UserRole).value<Contact>();

    // 绘制头像
    QRect avatarRect(rect.left() + margin, rect.center().y()-avatarSize/2, avatarSize, avatarSize);
    ThumbnailResourceManager* mediaManager = ThumbnailResourceManager::instance();
    painter->setRenderHint(QPainter::Antialiasing,true);

    QPixmap avatar = mediaManager->getThumbnail(contact.user.avatar_local_pathValue(),
                                                QSize(avatarSize, avatarSize),
                                                MediaType::Avatar,
                                                5,
                                                "",
                                                contact.user.nicknameValue()
                                                );

    if(!avatar.isNull()) {
        painter->drawPixmap(avatarRect, avatar);
    } else {
        drawDefaultAvatar(painter, avatarRect, contact.user.nicknameValue(), 5);
    }

    // 绘制文本
    QString text = contact.remark_nameValue();
    if(text.isEmpty()) text = contact.user.nicknameValue();
    QRect textRect(avatarRect.right() + margin, avatarRect.top(),
                   rect.width()-avatarSize-margin*3, avatarSize);

    // 设置文本颜色
    painter->setPen(QColor(0, 0, 0));  // 恢复黑色
    QFont font = option.font;
    font.setPointSizeF(11);
    font.setFamily(QStringLiteral("微软雅黑"));
    painter->setFont(font);

    // 绘制文本
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

QSize ContactItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    if(m_model->isParentNode(index)){
        return QSize(option.rect.width(), 36);
    }
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
