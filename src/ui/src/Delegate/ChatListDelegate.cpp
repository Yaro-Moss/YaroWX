#include "ChatListDelegate.h"
#include "ContactController.h"
#include "Conversation.h"
#include <QPainter>
#include <QDateTime>
#include <QPainterPath>
#include <QAbstractItemView>
#include <QSize>
#include "FormatTime.h"

ChatListDelegate::ChatListDelegate(QObject *parent) 
    : QStyledItemDelegate(parent)
{
    ThumbnailResourceManager* thumbnailManager = ThumbnailResourceManager::instance();
    connect(thumbnailManager, &ThumbnailResourceManager::mediaLoaded,
            this, &ChatListDelegate::onMediaLoaded);
}

ChatListDelegate::~ChatListDelegate()
{
}

void ChatListDelegate::paint(QPainter *painter, const QStyleOptionViewItem
                                                    &option, const QModelIndex &index )const
{
    painter->save();

    const int topBottomMargin = 15;
    const int leftRightMargin = 12;
    const int avatarSize = 40;
    const int spacing = 12;

    QRect rect = option.rect;

    // 选中灰色，悬停浅灰
    if(option.state & QStyle::State_Selected){
        painter->fillRect(rect, QColor(226,226,226));
    }else if(option.state & QStyle::State_MouseOver){
        painter->fillRect(rect, QColor(238,238,238));
    }

    // 获取数据
    QString name = index.data(TitleRole).toString();
    QString lastMsg = index.data(LastMessageContentRole).toString();
    QString timeText = FormatTime(index.data(LastMessageTimeRole).toLongLong());
    int unreadCount = index.data(UnreadCountRole).toInt();
    QString avatarPath = index.data(AvatarLocalPathRole).toString();
    qint64 id = index.data(TargetIdRole).toLongLong();
    QString avatarText = QString();
    if(m_contactController) avatarText = m_contactController->getContactFromModel(id).user.nicknameValue();

    if(avatarPath.isEmpty()) {
        avatarPath = index.data(AvatarRole).toString();
    }

    QRect avatarRect(rect.left()+leftRightMargin, rect.center().y()-avatarSize/2, avatarSize, avatarSize);
    
    QFont timeFont = option.font;
    timeFont.setPointSizeF(7.5);
    timeFont.setFamily(QStringLiteral("微软雅黑"));
    QFontMetrics timeFm(timeFont);
    int timeW = timeFm.horizontalAdvance(timeText);
    QRect timeRect(rect.right()-leftRightMargin-timeW, rect.top()+topBottomMargin, timeW, timeFm.height());

    int left = avatarRect.right()+spacing;
    int right = timeRect.left()-spacing;
    QRect nameRect(left, rect.top()+topBottomMargin, right-left, option.fontMetrics.height());
    QRect msgRect(left,rect.bottom()-topBottomMargin-option.fontMetrics.height(),right-left,option.fontMetrics.height());

    //画-name
    QFont nameFont = option.font;
    nameFont.setPointSizeF(10.5);
    nameFont.setFamily(QStringLiteral("微软雅黑"));
    painter->setFont(nameFont);
    painter->setPen(option.palette.text().color());
    QString elidedName = QFontMetrics(nameFont).elidedText(name,Qt::ElideRight, nameRect.width());
    painter->drawText(nameRect, Qt::AlignLeft|Qt::AlignVCenter, elidedName);

    //画-lastMsg
    QFont msgFont = option.font;
    msgFont.setPointSizeF(8);
    msgFont.setFamily(QStringLiteral("微软雅黑"));
    painter->setFont(msgFont);
    painter->setPen(QColor(150,150,150));
    QString elidedMsg = QFontMetrics(msgFont).elidedText(lastMsg, Qt::ElideRight, msgRect.width());
    painter->drawText(msgRect, Qt::AlignLeft|Qt::AlignVCenter,elidedMsg);

    // 画-lastTime
    painter->setFont(timeFont);
    painter->setPen(QColor(150,150,150));
    painter->drawText(timeRect, Qt::AlignLeft|Qt::AlignVCenter,timeText);

    //画-头像
    ThumbnailResourceManager* thumbnailManager = ThumbnailResourceManager::instance();
    painter->setRenderHint(QPainter::Antialiasing,true);

    QPixmap avatar = thumbnailManager->getThumbnail(avatarPath,
                                                    QSize(avatarSize, avatarSize),
                                                    MediaType::Avatar,
                                                    5, "", avatarText);


    if(!avatar.isNull()) {
        painter->drawPixmap(avatarRect, avatar);
    } else {
        drawDefaultAvatar(painter, avatarRect, name, 5);
    }

    //画未读
    if(unreadCount>0){
        QString badgetText = (unreadCount>99)? QStringLiteral("99+") : QString::number(unreadCount);
        QFont badgetFont = option.font;
        badgetFont.setBold(true);
        badgetFont.setPointSizeF(7.5);

        QFontMetrics bf(badgetFont);
        int textW = bf.horizontalAdvance(badgetText);
        int textH = bf.height();

        //内边距
        const int hPadding = 3;
        const int vpadding = 2;

        int badgeW = textW + hPadding*2;
        int badgeH = textH + vpadding*2;

        if(badgeW<badgeH)badgeW = badgeH;

        int bx = avatarRect.right()-badgeW/2;
        int by = avatarRect.top()-badgeH/4;

        QRect badgeRect(bx,by, badgeW, badgeH);

        //pill背景
        painter->setRenderHint(QPainter::Antialiasing,true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(249,81,81));
        painter->drawRoundedRect(badgeRect, badgeH/2.0, badgeH/2.0);

        //绘制文字
        painter->setFont(badgetFont);
        painter->setPen(Qt::white);
        painter->drawText(badgeRect, Qt::AlignCenter, badgetText);
    }

    // 画置顶标识（可选）
    bool isTop = index.data(IsTopRole).toBool();
    if(isTop) {
        // 在会话项右上角添加置顶标识
        QRect topRect(timeRect.left() - 20, timeRect.top(), 16, 16);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 193, 7));
        painter->drawEllipse(topRect);

        QFont topFont = option.font;
        topFont.setPointSizeF(6);
        topFont.setBold(true);
        painter->setFont(topFont);
        painter->setPen(Qt::white);
        painter->drawText(topRect, Qt::AlignCenter, "顶");
    }

    painter->restore();
}

QSize ChatListDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index)const {
    Q_UNUSED(index)
    return QSize(option.rect.width(), 69);
}


void ChatListDelegate::onMediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type)
{
    if(type == MediaType::Avatar) {
        // 通知视图更新
        if(QAbstractItemView* view = qobject_cast<QAbstractItemView*>(parent())) {
            view->viewport()->update();
        }
    }
}

void ChatListDelegate::drawDefaultAvatar(QPainter *painter, const QRect &avatarRect, 
                                        const QString &name, int radius) const
{
    QPainterPath path;
    path.addRoundedRect(QRectF(avatarRect), radius, radius);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(210,210,210));
    painter->drawPath(path);

    QFont iniFont = painter->font();
    iniFont.setBold(true);
    iniFont.setPointSize(15);
    painter->setFont(iniFont);
    painter->setPen(QColor(100,100,100));
    painter->drawText(avatarRect, Qt::AlignCenter, name.isEmpty() ? "?" : name.left(1));
}
