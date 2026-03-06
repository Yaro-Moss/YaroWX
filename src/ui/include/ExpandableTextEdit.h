#ifndef EXPANDABLETEXTEDIT_H
#define EXPANDABLETEXTEDIT_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <qevent.h>

class ExpandableTextEdit : public QWidget
{
    Q_OBJECT
public:
    explicit ExpandableTextEdit(QWidget *parent = nullptr);
    ExpandableTextEdit(const QString &text, QWidget *parent = nullptr);

    // 对外接口
    QString text() const;
    void setText(const QString &text);
    int foldHeight() const;
    void setFoldHeight(int height);
    void setFont(const QFont &font);
    void setFontSize(int size);
    bool isExpanded() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void toggleExpand();
    void checkTextHeight();

private:
    void initUI();
    int calculateTextHeight() const;

    // 自定义无滚动 QTextEdit
    class NoScrollTextEdit : public QTextEdit
    {
    public:
        NoScrollTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {
            setReadOnly(true);
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }

    protected:
        // 彻底禁用滚轮
        void wheelEvent(QWheelEvent *event) override {
            event->ignore();  // 忽略滚轮事件
        }

        // 禁用所有会导致滚动的按键（仅保留复制快捷键）
        void keyPressEvent(QKeyEvent *event) override {
            if (event == QKeySequence::Copy) {
                QTextEdit::keyPressEvent(event);  // 允许复制
            } else {
                event->ignore();  // 其他按键忽略
            }
        }

        // 阻止内部滚动（防止因光标移动导致的自动滚动）
        void scrollContentsBy(int dx, int dy) override {
            // 什么都不做，彻底阻止滚动
            Q_UNUSED(dx);
            Q_UNUSED(dy);
        }
    };

    NoScrollTextEdit *m_textEdit;
    QPushButton *m_toggleBtn;
    int m_foldHeight;
    bool m_isExpanded;
};

#endif // EXPANDABLETEXTEDIT_H
