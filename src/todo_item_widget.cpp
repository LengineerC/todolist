#include "todo_item_widget.h"
#include "config_manager.h"
#include "constants.h"
#include "utils.h"

#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>

namespace {
constexpr int kItemMinHeight = 56;
constexpr int kLongPressMs = 200;
} // namespace

TodoItemWidget::TodoItemWidget(const QString &text, QWidget *parent)
    : QWidget(parent), m_label(new QLabel(this)), m_editor(new QLineEdit(this)),
      m_cancelButton(new QPushButton(this)), m_pressing(false),
      m_longPressActive(false), m_editing(false), m_text(text) {

    m_cancelButton->setFocusPolicy(Qt::NoFocus);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_label->setTextInteractionFlags(Qt::NoTextInteraction);
    m_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_label->setMinimumWidth(0);
    QFont textFont = m_label->font();
    textFont.setPixelSize(Config::contentTextFontPx);
    m_label->setFont(textFont);
    m_label->setText(text);
    applyLabelStyle();

    m_editor->setText(text);
    m_editor->setFont(textFont);
    m_editor->setVisible(false);
    m_editor->installEventFilter(this);

    m_cancelButton->setVisible(false);
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setFocusPolicy(Qt::NoFocus);
    m_cancelButton->setFixedSize(22, 22);

    layout->addWidget(m_label, 1);
    layout->addWidget(m_editor, 1);
    layout->addWidget(m_cancelButton, 0, Qt::AlignCenter);

    connect(m_editor, &QLineEdit::returnPressed, this,
            [this]() { finishInlineEdit(true); });
    connect(m_editor, &QLineEdit::editingFinished, this,
            [this]() { finishInlineEdit(true); });
    connect(m_cancelButton, &QPushButton::clicked, this,
            [this]() { finishInlineEdit(false); });

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(kItemMinHeight);
    setMaximumHeight(kItemMinHeight);

    m_longPressTimer.setSingleShot(true);
    connect(&m_longPressTimer, &QTimer::timeout, this, [this]() {
        if (!m_pressing || m_longPressActive || m_editing) {
            return;
        }

        m_longPressActive = true;
        emit longPressStarted(this, QCursor::pos());
    });

    updateDynamicHeight();
    applyTheme();
}

QString TodoItemWidget::text() const { return m_text; }

void TodoItemWidget::setText(const QString &text) {
    m_text = text;
    m_label->setText(formatDisplayText(m_text, qMax(1, width() - 24)));
    if (!m_editing) {
        m_editor->setText(m_text);
    }
    updateDynamicHeight();
}

void TodoItemWidget::setTextHidden(bool hidden) {
    m_label->setVisible(!hidden);
    if (hidden) {
        m_label->setStyleSheet("color: rgba(0,0,0,0);");
    } else {
        applyLabelStyle();
    }
}

int TodoItemWidget::preferredItemHeight() const {
    const int targetWidth = qMax(1, width() - 24);
    const QString displayText = formatDisplayText(m_text, targetWidth);

    QFontMetrics fm(m_label->font());
    const QRect textRect = fm.boundingRect(QRect(0, 0, targetWidth, 100000),
                                           wrapFlags(), displayText);

    return qMax(kItemMinHeight, textRect.height() + 20);
}

bool TodoItemWidget::isEditing() const { return m_editing; }

void TodoItemWidget::mousePressEvent(QMouseEvent *event) {
    if (m_editing) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::RightButton) {
        beginInlineEdit();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_pressing = true;
        m_longPressActive = false;
        m_pressPos = event->pos();
        m_longPressTimer.start(kLongPressMs);
    }

    QWidget::mousePressEvent(event);
}

void TodoItemWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_editing) {
        QWidget::mouseMoveEvent(event);
        return;
    }

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

    if (!m_editing && m_pressing && m_longPressActive) {
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

bool TodoItemWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_editor && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            finishInlineEdit(false);
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TodoItemWidget::updateDynamicHeight() {
    const int targetWidth = qMax(1, width() - 24);
    const QString displayText = formatDisplayText(m_text, targetWidth);

    m_label->setText(displayText);

    QFontMetrics fm(m_label->font());
    const QRect textRect = fm.boundingRect(QRect(0, 0, targetWidth, 100000),
                                           wrapFlags(), displayText);

    const int totalHeight = qMax(kItemMinHeight, textRect.height() + 20);

    m_label->setMinimumHeight(textRect.height());
    m_label->setMaximumHeight(textRect.height());
    setMinimumHeight(totalHeight);
    setMaximumHeight(totalHeight);
    updateGeometry();
}

void TodoItemWidget::beginInlineEdit() {
    if (m_editing) {
        return;
    }

    m_editing = true;
    m_pressing = false;
    m_longPressActive = false;
    m_longPressTimer.stop();

    m_editOriginalText = m_text;
    m_editor->setText(m_editOriginalText);
    m_label->hide();
    m_editor->show();
    m_cancelButton->show();
    m_editor->setFocus();
    m_editor->selectAll();
}

void TodoItemWidget::finishInlineEdit(bool confirm) {
    if (!m_editing) {
        return;
    }

    QString nextText = m_editOriginalText;
    if (confirm) {
        const QString trimmed = m_editor->text().trimmed();
        if (!trimmed.isEmpty()) {
            nextText = trimmed;
        }
    }

    m_text = nextText;
    m_label->setText(formatDisplayText(m_text, qMax(1, width() - 24)));
    m_editor->setText(m_text);

    m_editor->hide();
    m_cancelButton->hide();
    m_label->show();
    m_editing = false;

    updateDynamicHeight();
}

QString TodoItemWidget::formatDisplayText(const QString &text,
                                          int maxWidth) const {
    if (text.isEmpty() || !forceWrapEnabled()) {
        return text;
    }

    QFontMetrics fm(m_label->font());
    QString result;
    result.reserve(text.size() + text.size() / 16);

    int runWidth = 0;
    for (const QChar ch : text) {
        if (ch == '\n') {
            result.append(ch);
            runWidth = 0;
            continue;
        }

        const int charWidth = fm.horizontalAdvance(ch);
        if (runWidth > 0 && runWidth + charWidth > maxWidth) {
            result.append('\n');
            runWidth = 0;
        }

        result.append(ch);
        runWidth += charWidth;
    }

    return result;
}

int TodoItemWidget::wrapFlags() const {
    int flags = Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop;
    if (forceWrapEnabled()) {
        flags |= Qt::TextWrapAnywhere;
    }
    return flags;
}

bool TodoItemWidget::forceWrapEnabled() const {
    const QString wrapMode =
        ConfigManager::instance().getConfig()["todoWrapMode"].toString("force");
    return wrapMode != "word";
}

void TodoItemWidget::applyTheme() {
    const QString theme =
        ConfigManager::instance().getConfig()["theme"].toString("light");
    const QString inputTextColor =
        Utils::colorToRgba(Config::Themes::getTheme(theme).textColor, 255);
    const QString inputBgColor =
        Utils::colorToRgba(Config::Themes::getTheme(theme).backgroundColor, 40);
    const QString inputBorderColor = Utils::colorToRgba(
        Config::Themes::getReverseTheme(theme).backgroundColor, 90);
    m_editor->setStyleSheet(
        QString("QLineEdit {"
                " color: %1;"
                " background: %2;"
                " border: 1px solid %3;"
                " border-radius: 6px;"
                "}")
            .arg(inputTextColor, inputBgColor, inputBorderColor));

    QColor iconColor = Config::Themes::getTheme(theme).textColor;
    iconColor.setAlpha(100);
    m_cancelButton->setIcon(Utils::getColoredSvg(":/icons/close", iconColor));

    const QString btnBgColor = Utils::colorToRgba(
        Config::Themes::getReverseTheme(theme).backgroundColor, 100);
    m_cancelButton->setStyleSheet(QString("QPushButton {"
                                          " background: transparent;"
                                          " border-radius: 2px;"
                                          " height: 100%;"
                                          "}"
                                          "QPushButton:hover {"
                                          " background: %1;"
                                          "}")
                                      .arg(btnBgColor));

    applyLabelStyle();
}

void TodoItemWidget::applyLabelStyle() {
    const QString textColor = Utils::colorToRgba(
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor,
        255);
    const QString textHoverColor = Utils::colorToRgba(
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
}
