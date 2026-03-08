#ifndef CLICKCLOSEPOPUP_H
#define CLICKCLOSEPOPUP_H

#include <QDialog>
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QKeyEvent>

class ClickClosePopup : public QDialog
{
    Q_OBJECT

public:
    explicit ClickClosePopup(QWidget *parent = nullptr)
        : QDialog(parent)
        , m_clickCloseEnabled(false)
        // 保存QDialog原始属性（修复attribute错误，使用testAttribute）
        , m_originalWindowFlags(windowFlags())
        , m_originalTranslucentAttr(testAttribute(Qt::WA_TranslucentBackground))
        , m_originalDeleteOnCloseAttr(testAttribute(Qt::WA_DeleteOnClose))
    {
        // 初始为普通QDialog状态，无特殊设置
    }

    virtual ~ClickClosePopup()
    {
        // 析构时移除事件过滤器
        if (m_clickCloseEnabled) {
            qApp->removeEventFilter(this);
        }
    }

    // 启用「无边框+点击外部关闭+圆角+Esc关闭+透明背景+关闭自动删除」功能
    void enableClickCloseFeature()
    {
        if (m_clickCloseEnabled) return;

        // 首次启用保存原始状态
        if (!m_hasSavedOriginalState) {
            m_originalWindowFlags = windowFlags();
            m_originalTranslucentAttr = testAttribute(Qt::WA_TranslucentBackground);
            m_originalDeleteOnCloseAttr = testAttribute(Qt::WA_DeleteOnClose);
            m_hasSavedOriginalState = true;
        }

        // 1. 弹窗态：开启关闭自动删除
        setAttribute(Qt::WA_DeleteOnClose, true);

        // 2. 切换为无边框 + 透明背景（QDialog专属窗口标志组合）
        setWindowFlags(m_originalWindowFlags | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground, true);
        show(); // 应用窗口标志

        // 3. 启用全局事件过滤
        m_clickCloseEnabled = true;
        qApp->installEventFilter(this);

        // 4. 触发重绘（显示圆角）
        update();
    }

    // 禁用功能，回归普通QDialog状态
    void disableClickCloseFeature()
    {
        if (!m_clickCloseEnabled) return;

        // 1. 移除事件过滤器
        m_clickCloseEnabled = false;
        qApp->removeEventFilter(this);

        // 2. 恢复原始删除属性
        setAttribute(Qt::WA_DeleteOnClose, m_originalDeleteOnCloseAttr);

        // 3. 恢复QDialog原始窗口标志和透明属性
        setWindowFlags(m_originalWindowFlags);
        setAttribute(Qt::WA_TranslucentBackground, m_originalTranslucentAttr);
        show(); // 应用窗口标志

        // 4. 触发重绘（恢复原生背景）
        update();
    }

    // 检查功能是否启用
    bool isClickCloseEnabled() const { return m_clickCloseEnabled; }

    // 位置自适应显示（适配QDialog）
    void showAtPos(const QPoint &pos)
    {
        QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
        QPoint adjustedPos = pos;

        if (adjustedPos.x() + width() > screenGeometry.right()) {
            adjustedPos.setX(screenGeometry.right() - width());
        }
        if (adjustedPos.y() + height() > screenGeometry.bottom()) {
            adjustedPos.setY(screenGeometry.bottom() - height());
        }

        move(adjustedPos);
        show();
    }

protected:
    // 事件过滤器：仅弹窗态处理外部点击
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (!m_clickCloseEnabled) {
            return QDialog::eventFilter(obj, event); // 回归QDialog默认处理
        }

        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            if (isVisible() && !geometry().contains(globalPos)) {
                close();
                return true;
            }
        }

        return QDialog::eventFilter(obj, event);
    }

    // 绘制事件：弹窗态绘圆角，普通态回归QDialog原生样式
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (m_clickCloseEnabled) {
            // 弹窗态：圆角透明背景（和原逻辑一致）
            painter.fillRect(rect(), Qt::transparent);
            QPen pen(QColor(160, 160, 160), 1);
            painter.setPen(pen);
            painter.setBrush(QColor(255, 255, 255));
            QRect contentRect(1, 1, width()-2, height()-2);
            painter.drawRoundedRect(contentRect, 6, 6);
        } else {
            // 普通态：调用QDialog原生绘制
            QDialog::paintEvent(event);
        }
    }

    // 按键事件：仅弹窗态响应Esc关闭
    void keyPressEvent(QKeyEvent *event) override
    {
        if (m_clickCloseEnabled && event->key() == Qt::Key_Escape) {
            close();
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    bool m_clickCloseEnabled;          // 功能开关
    Qt::WindowFlags m_originalWindowFlags; // QDialog原始窗口标志
    bool m_originalTranslucentAttr;    // 原始透明属性
    bool m_originalDeleteOnCloseAttr;  // 原始WA_DeleteOnClose属性
    bool m_hasSavedOriginalState = false; // 标记是否保存过原始状态
};

#endif // CLICKCLOSEPOPUP_H
