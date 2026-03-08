#ifndef CHATLISTDELEGATE_H
#define CHATLISTDELEGATE_H

#include <QStyledItemDelegate>
#include "ThumbnailResourceManager.h"

class ContactController;

class ChatListDelegate : public QStyledItemDelegate 
{
    Q_OBJECT

public:
    ChatListDelegate(QObject *parent = nullptr);
    ~ChatListDelegate();
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option, 
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setContactController(ContactController *contactController){m_contactController=contactController;}

private slots:
    void onMediaLoaded(const QString& resourcePath, const QPixmap& media, MediaType type);

private:
    void drawDefaultAvatar(QPainter *painter, const QRect &avatarRect, 
                          const QString &name, int radius) const;

    ContactController *m_contactController;

};

#endif // CHATLISTDELEGATE_H
