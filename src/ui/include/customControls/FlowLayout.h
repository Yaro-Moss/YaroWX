#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QRect>
#include <QStyle>
#include <QWidget>

struct Margins
{
    Margins(int l = -1, int t = -1, int r =-1, int b = -1) {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }
    int left = -1;
    int top = -1;
    int right = -1;
    int bottom = -1;
};

class FlowLayout : public QLayout
{
    Q_OBJECT
public:
    // 构造函数，支持设置边距和间距
    explicit FlowLayout(QWidget *parent = nullptr, Margins margins = Margins() , int hSpacing = -1, int vSpacing = -1)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margins.left, margins.top, margins.right, margins.bottom);
    }

    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    // 析构函数，释放所有布局项
    ~FlowLayout() override
    {
        QLayoutItem *item;
        while ((item = takeAt(0))) delete item;
    }

    // 布局核心：添加控件/布局项
    void addItem(QLayoutItem *item) override { m_items.append(item); }
    // 水平间距
    int horizontalSpacing() const
    {
        return m_hSpace >= 0 ? m_hSpace : smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
    // 垂直间距
    int verticalSpacing() const
    {
        return m_vSpace >= 0 ? m_vSpace : smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
    // 布局方向（流式为水平）
    Qt::Orientations expandingDirections() const override { return Qt::Horizontal; }
    // 是否为空
    bool hasHeightForWidth() const override { return true; }
    // 宽高对应（核心：根据宽度计算高度）
    int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
    // 布局项数量
    int count() const override { return m_items.size(); }
    // 获取指定位置的布局项
    QLayoutItem *itemAt(int index) const override { return m_items.value(index); }
    // 移除并返回指定位置的布局项
    QLayoutItem *takeAt(int index) override { return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr; }
    // 设置布局几何（核心：实时重排控件位置）
    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }
    // 推荐大小
    QSize sizeHint() const override { return minimumSize(); }
    // 最小大小
    QSize minimumSize() const override
    {
        QSize size;
        for (QLayoutItem *item : m_items)
            size = size.expandedTo(item->minimumSize());
        const QMargins margins = contentsMargins();
        size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
        return size;
    }

private:
    // 智能间距（适配系统样式）
    int smartSpacing(QStyle::PixelMetric pm) const
    {
        QObject *parent = this->parent();
        if (!parent) return -1;
        if (parent->isWidgetType()) {
            QWidget *pw = static_cast<QWidget*>(parent);
            return pw->style()->pixelMetric(pm, nullptr, pw);
        } else {
            return static_cast<QLayout*>(parent)->spacing();
        }
    }

    // 实际布局逻辑：rect=布局区域，testOnly=是否仅计算高度（不重排）
    int doLayout(const QRect &rect, bool testOnly) const
    {
        const int left = contentsMargins().left();
        const int top = contentsMargins().top();
        const int right = contentsMargins().right();
        const int bottom = contentsMargins().bottom();
        // 可用布局区域
        QRect effectiveRect = rect.adjusted(left, top, -right, -bottom);
        int x = effectiveRect.x();
        int y = effectiveRect.y();
        int lineHeight = 0; // 当前行的高度

        for (QLayoutItem *item : m_items) {
            const QWidget *wid = item->widget();
            // 计算控件的水平/垂直间距
            int spaceX = horizontalSpacing();
            if (spaceX == -1) spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
            int spaceY = verticalSpacing();
            if (spaceY == -1) spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
            // 控件的目标位置
            int nextX = x + item->sizeHint().width() + spaceX;
            // 关键：如果下一个控件超出右边界，换行
            if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
                x = effectiveRect.x();
                y = y + lineHeight + spaceY;
                nextX = x + item->sizeHint().width() + spaceX;
                lineHeight = 0;
            }
            // 非测试模式：设置控件几何位置
            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            // 更新当前行的x坐标和行高
            x = nextX;
            lineHeight = qMax(lineHeight, item->sizeHint().height());
        }
        // 返回整个布局的总高度
        return y + lineHeight - rect.y() + bottom;
    }

    QList<QLayoutItem *> m_items; // 存储所有布局项
    int m_hSpace; // 水平间距
    int m_vSpace; // 垂直间距
};

#endif // FLOWLAYOUT_H
