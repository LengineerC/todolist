#include "todo_item_widget.h"
#include "config_manager.h"
#include "constants.h"
#include "utils.h"

#include <QApplication>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMouseEvent>

namespace {
constexpr int kItemMinHeight = 56;
constexpr int kLongPressMs = 320;
} // namespace

TodoItemWidget::TodoItemWidget(const QString &text, QWidget *parent)
    : QWidget(parent), m_label(new QLabel(this)), m_pressing(false),
      m_longPressActive(false) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);

    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_label->setText(text);
    QString textColor = Utils::colorToRgba(
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor,
        255);
    QString textHoverColor = Utils::colorToRgba(
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor,
        150);
    m_label->setStyleSheet(QString("QLabel {"
                                   " color: %1;"
                                   " border-radius: 3px;"
                                   "}"
                                   "QLabel:hover {"
                                   " color: %2;"
                                   "}")
                               .arg(textColor, textHoverColor));
    layout->addWidget(m_label);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(kItemMinHeight);
    setMaximumHeight(kItemMinHeight);

    m_longPressTimer.setSingleShot(true);
    connect(&m_longPressTimer, &QTimer::timeout, this, [this]() {
        if (!m_pressing || m_longPressActive) {
            return;
        }

        m_longPressActive = true;
        emit longPressStarted(this, QCursor::pos());
    });

    updateDynamicHeight();
}

QString TodoItemWidget::text() const { return m_label->text(); }

void TodoItemWidget::setText(const QString &text) {
    m_label->setText(text);
    updateDynamicHeight();
}

void TodoItemWidget::setTextHidden(bool hidden) {
    m_label->setVisible(!hidden);
    if (hidden) {
        m_label->setStyleSheet("color: rgba(0,0,0,0);");
    } else {
        m_label->setStyleSheet("");
    }
}

int TodoItemWidget::preferredItemHeight() const {
    const int targetWidth = qMax(1, width() - 24);

    QFontMetrics fm(m_label->font());
    const QRect textRect = fm.boundingRect(
        QRect(0, 0, targetWidth, 100000),
        Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, m_label->text());

    return qMax(kItemMinHeight, textRect.height() + 20);
}

void TodoItemWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_pressing = true;
        m_longPressActive = false;
        m_pressPos = event->pos();
        m_longPressTimer.start(kLongPressMs);
    }

    QWidget::mousePressEvent(event);
}

void TodoItemWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!m_pressing) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (!m_longPressActive) {
        const int distance = (event->pos() - m_pressPos).manhattanLength();
        if (distance > QApplication::startDragDistance()) {
            m_longPressTimer.stop();
        }
    } else {
        emit dragMoved(this, event->globalPos());
    }

    QWidget::mouseMoveEvent(event);
}

void TodoItemWidget::mouseReleaseEvent(QMouseEvent *event) {
    m_longPressTimer.stop();

    if (m_pressing && m_longPressActive) {
        emit dragReleased(this, event->globalPos());
    }

    m_pressing = false;
    m_longPressActive = false;
    QWidget::mouseReleaseEvent(event);
}

void TodoItemWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(this);
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TodoItemWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateDynamicHeight();
}

void TodoItemWidget::updateDynamicHeight() {
    const int targetWidth = qMax(1, width() - 24);

    QFontMetrics fm(m_label->font());
    const QRect textRect = fm.boundingRect(
        QRect(0, 0, targetWidth, 100000),
        Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, m_label->text());

    const int totalHeight = qMax(kItemMinHeight, textRect.height() + 20);

    m_label->setMinimumHeight(textRect.height());
    m_label->setMaximumHeight(textRect.height());
    setMinimumHeight(totalHeight);
    setMaximumHeight(totalHeight);
    updateGeometry();
}
