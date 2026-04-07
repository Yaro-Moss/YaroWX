#ifndef CONTACTTREEVIEW_H
#define CONTACTTREEVIEW_H

#include <QTreeView>
#include "Contact.h"
#include "CustomTreeView.h"

class ContactTreeModel;

class ContactTreeView : public CustomTreeView
{
    Q_OBJECT

public:
    explicit ContactTreeView(QWidget *parent = nullptr);

    Contact getSelectedContact() const;
    Contact getContactFromIndex(const QModelIndex &index) const;

    void setContactModel(ContactTreeModel *model);
    void drawRow(QPainter *painter,
                 const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;

signals:
    void sendMessage(const Contact &contact);
    void starFriend(const Contact &contact);
    void removeFriend(const Contact &contact);



protected:
    // 重写鼠标事件处理
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void leaveEvent(QEvent *event) override;
    void drawBranch(QPainter *painter, const QRect &rect, const QModelIndex &index) const ;

private:
    ContactTreeModel *m_contactModel;

    // 处理节点点击
    void handleParentNodeClick(const QModelIndex &index);
    void createContactMenu();
    void showDeleteConfirmationDialog();


    QMenu *contactMenu;
    QAction* sendMessageAction;
    QAction* starFriendAction;
    QAction *removeFriendAction;
    mutable QPersistentModelIndex m_lastHoverIndex;  // 记录上次悬停的项

};

#endif // CONTACTTREEVIEW_H
