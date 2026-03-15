#include "ExpandableTextEdit.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QTextDocument>
#include <QTimer>
#include <QResizeEvent>
#include <QKeyEvent>

ExpandableTextEdit::ExpandableTextEdit(QWidget *parent)
    : QWidget(parent)
    , m_foldHeight(100)
    , m_isExpanded(false)
{
    initUI();
}

ExpandableTextEdit::ExpandableTextEdit(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_foldHeight(100)
    , m_isExpanded(false)
{
    initUI();
    setText(text);
}

void ExpandableTextEdit::initUI()
{
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("QWidget { background: transparent; }");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // 使用自定义无滚动 QTextEdit
    m_textEdit = new NoScrollTextEdit(this);
    m_textEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    m_textEdit->setFrameShape(QTextEdit::NoFrame);
    m_textEdit->setAttribute(Qt::WA_TranslucentBackground);
    m_textEdit->setStyleSheet(R"(
        QTextEdit {
            font: 17px "微软雅黑";
            color: rgb(76, 76, 76);
            background: transparent;
            border: none;
            padding: 0;
        }
        QTextEdit::viewport { background: transparent; }
    )");

    // 按钮
    m_toggleBtn = new QPushButton("展开 ▼", this);
    m_toggleBtn->setStyleSheet(R"(
        QPushButton {
            font: 12px "微软雅黑";
            color: rgb(76, 76, 76);
            background: transparent;
            border: none;
            outline: none;
            padding: 5px 10px;
        }
        QPushButton:hover { color: rgb(50, 50, 50); }
        QPushButton:pressed { color: rgb(30, 30, 30); }
    )");
    m_toggleBtn->hide();

    // 布局
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_toggleBtn);
    btnLayout->setContentsMargins(0, 5, 0, 0);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_textEdit);
    mainLayout->addLayout(btnLayout);

    connect(m_toggleBtn, &QPushButton::clicked, this, &ExpandableTextEdit::toggleExpand);
}

int ExpandableTextEdit::calculateTextHeight() const
{
    if (!m_textEdit || m_textEdit->toPlainText().isEmpty()) {
        return 0;
    }

    QTextDocument doc;
    doc.setPlainText(m_textEdit->toPlainText());
    doc.setDefaultFont(m_textEdit->font());

    int textWidth = m_textEdit->width();
    if (textWidth <= 0) {
        textWidth = (m_textEdit->parentWidget() ? m_textEdit->parentWidget()->width() - 40 : 500);
    }
    doc.setTextWidth(textWidth);

    return qRound(doc.size().height());
}

void ExpandableTextEdit::checkTextHeight()
{
    int realTextHeight = calculateTextHeight();

    if (realTextHeight <= m_foldHeight) {
        m_toggleBtn->hide();
        m_isExpanded = false;
        m_textEdit->setFixedHeight(realTextHeight);
    } else {
        m_toggleBtn->show();
        if (!m_isExpanded) {
            m_textEdit->setFixedHeight(m_foldHeight);
            m_toggleBtn->setText("展开 ▼");
        } else {
            m_textEdit->setFixedHeight(realTextHeight);
            m_toggleBtn->setText("收起 ▲");
        }
    }
    updateGeometry();
}

void ExpandableTextEdit::toggleExpand()
{
    m_isExpanded = !m_isExpanded;
    int realTextHeight = calculateTextHeight();

    if (realTextHeight > m_foldHeight) {
        if (m_isExpanded) {
            m_textEdit->setFixedHeight(realTextHeight);
            m_toggleBtn->setText("收起 ▲");
        } else {
            m_textEdit->setFixedHeight(m_foldHeight);
            m_toggleBtn->setText("展开 ▼");
        }
    }
    updateGeometry();
}

void ExpandableTextEdit::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    checkTextHeight();
}

// ========== 对外接口 ==========

QString ExpandableTextEdit::text() const
{
    return m_textEdit->toPlainText();
}

void ExpandableTextEdit::setText(const QString &text)
{
    if (!m_textEdit) return;
    m_textEdit->setText(text);
    QTimer::singleShot(0, this, &ExpandableTextEdit::checkTextHeight);
}

int ExpandableTextEdit::foldHeight() const
{
    return m_foldHeight;
}

void ExpandableTextEdit::setFoldHeight(int height)
{
    if (height > 0 && height != m_foldHeight) {
        m_foldHeight = height;
        checkTextHeight();
    }
}

void ExpandableTextEdit::setFont(const QFont &font)
{
    m_textEdit->setFont(font);
    checkTextHeight();
}

void ExpandableTextEdit::setFontSize(int size)
{
    if (size > 0) {
        QFont font = m_textEdit->font();
        font.setPointSize(size);
        setFont(font);
    }
}

bool ExpandableTextEdit::isExpanded() const
{
    return m_isExpanded;
}
