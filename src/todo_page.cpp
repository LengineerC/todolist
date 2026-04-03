#include "todo_page.h"

#include "config_manager.h"
#include "constants.h"
#include "db_manager.h"
#include "todo_item_widget.h"
#include "utils.h"

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QVBoxLayout>

namespace {
constexpr int kItemMinHeight = 56;
constexpr int kListSpacing = 8;
constexpr int kGapAnimMs = 150;
constexpr int kMoveAnimMs = 140;
constexpr int kAutoScrollEdgePx = 36;
constexpr int kAutoScrollStepPx = 10;
constexpr int kAutoScrollIntervalMs = 16;
} // namespace

TodoPage::TodoPage(QWidget *parent)
    : QWidget(parent), m_scrollArea(nullptr), m_scrollContent(nullptr),
      m_listLayout(nullptr), m_addArea(nullptr), m_addEditorRow(nullptr),
      m_addLineEdit(nullptr), m_addCancelButton(nullptr),
      m_addInlineActive(false), m_placeholderItem(nullptr),
      m_dragProxy(nullptr), m_dragActive(false), m_dragItem(nullptr),
      m_dragFromIndex(-1), m_placeholderIndex(-1) {
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("background: transparent;");

    m_scrollContent = new QWidget(m_scrollArea);
    m_listLayout = new QVBoxLayout(m_scrollContent);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(kListSpacing);

    m_placeholderItem = new TodoItemWidget("", m_scrollContent);
    m_placeholderItem->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_placeholderItem->setTextHidden(true);
    m_placeholderItem->setStyleSheet("background: transparent; border: none;");
    m_placeholderItem->setMinimumHeight(0);
    m_placeholderItem->setMaximumHeight(0);
    m_placeholderItem->hide();

    m_dragProxy = new QLabel(m_scrollArea->viewport());
    m_dragProxy->hide();
    m_dragProxy->setWordWrap(true);
    m_dragProxy->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dragProxy->setAttribute(Qt::WA_StyledBackground, true);
    m_dragProxy->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    QString proxyBgColor = Utils::colorToRgba(
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .backgroundColor,
        20);

    QColor baseTextColor =
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor;
    QString textColor = Utils::colorToRgba(baseTextColor, 150);

    QString borderColor = Utils::colorToRgba(
        Config::Themes::getReverseTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .backgroundColor,
        100);
    m_dragProxy->setStyleSheet(QString("background-color: %1;"
                                       "border: 2px solid %3;"
                                       "border-radius: 8px;"
                                       "padding: 5px 12px;"
                                       "color: %2;")
                                   .arg(proxyBgColor, textColor, borderColor));

    m_addArea = new QPushButton(m_scrollContent);
    QColor iconColor = baseTextColor;
    iconColor.setAlpha(100);
    QIcon addIcon = Utils::getColoredSvg(":/icons/add", iconColor);
    m_addArea->setIcon(addIcon);
    m_addArea->setCursor(Qt::PointingHandCursor);
    m_addArea->setFixedHeight(kItemMinHeight);
    m_addArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_addArea->setStyleSheet(QString("QPushButton {"
                                     " background: transparent;"
                                    //  " border: 1px dashed rgba(0, 0, 0, 70);"
                                     " border-radius: 8px;"
                                     "}"
                                     "QPushButton:hover {"
                                     " background: %1;"
                                     " border-color: rgba(0, 0, 0, 130);"
                                     "}")
                                 .arg(borderColor));

    connect(m_addArea, &QPushButton::clicked, this,
            [this]() { beginAddInline(); });

    m_addEditorRow = new QWidget(m_scrollContent);
    m_addEditorRow->setFixedHeight(kItemMinHeight);
    m_addEditorRow->hide();

    auto *addEditorLayout = new QHBoxLayout(m_addEditorRow);
    addEditorLayout->setContentsMargins(12, 10, 12, 10);
    addEditorLayout->setSpacing(8);

    m_addLineEdit = new QLineEdit(m_addEditorRow);
    m_addLineEdit->setPlaceholderText("New todo item");
    m_addLineEdit->installEventFilter(this);

    const QString inputTextColor = Utils::colorToRgba(
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor,
        255);
    // const QString inputBgColor = Utils::colorToRgba(
    //     Config::Themes::getTheme(
    //         ConfigManager::instance().getConfig()["theme"].toString())
    //         .backgroundColor,
    //     40);
    // const QString inputBorderColor = Utils::colorToRgba(
    //     Config::Themes::getReverseTheme(
    //         ConfigManager::instance().getConfig()["theme"].toString())
    //         .backgroundColor,
    //     100);

    m_addLineEdit->setStyleSheet(
        QString("QLineEdit {"
                " color: %1;"
                " height: 100%;"
                " background: transparent;"
                " border: none;"
                // " background: %2;"
                // " border: 1px solid %3;"
                // " border-radius: 6px;"
                // " padding: 6px 10px;"
                "}")
            .arg(inputTextColor));

    m_addCancelButton = new QPushButton(m_addEditorRow);
    QIcon cancelIcon = Utils::getColoredSvg(":/icons/close", iconColor);
    m_addCancelButton->setIcon(cancelIcon);
    m_addCancelButton->setCursor(Qt::PointingHandCursor);
    m_addCancelButton->setFocusPolicy(Qt::NoFocus);
    m_addCancelButton->setFixedSize(22, 22);
    m_addCancelButton->setStyleSheet(
        QString("QPushButton {"
                " background: transparent;"
                " border-radius: 2px;"
                " height: 100%;"
                "}"
                "QPushButton:hover {"
                " background: %1;"
                "}")
            .arg(borderColor));

    addEditorLayout->addWidget(m_addLineEdit, 1);
    addEditorLayout->addWidget(m_addCancelButton, 0, Qt::AlignCenter);

    connect(m_addLineEdit, &QLineEdit::returnPressed, this,
            [this]() { finishAddInline(true); });
    connect(m_addLineEdit, &QLineEdit::editingFinished, this,
            [this]() { finishAddInline(true); });
    connect(m_addCancelButton, &QPushButton::clicked, this,
            [this]() { finishAddInline(false); });

    qApp->installEventFilter(this);

    m_autoScrollTimer.setInterval(kAutoScrollIntervalMs);
    connect(&m_autoScrollTimer, &QTimer::timeout, this, [this]() {
        if (!m_dragActive) {
            return;
        }

        const QPoint viewportPos =
            m_scrollArea->viewport()->mapFromGlobal(m_lastDragGlobalPos);
        QScrollBar *bar = m_scrollArea->verticalScrollBar();
        int nextValue = bar->value();

        if (viewportPos.y() < kAutoScrollEdgePx) {
            nextValue -= kAutoScrollStepPx;
        } else if (viewportPos.y() >
                   m_scrollArea->viewport()->height() - kAutoScrollEdgePx) {
            nextValue += kAutoScrollStepPx;
        }

        nextValue = qBound(bar->minimum(), nextValue, bar->maximum());
        if (nextValue != bar->value()) {
            bar->setValue(nextValue);
            updateDrag(m_lastDragGlobalPos);
        }
    });

    m_scrollArea->setWidget(m_scrollContent);
    rootLayout->addWidget(m_scrollArea);

    DbManager::instance().init();
    const QVector<TodoRecord> todos = DbManager::instance().loadActiveTodos();
    for (const auto &todo : todos) {
        addTodoItem(todo.content, todo.id);
    }

    rebuildListLayout(false);
}

void TodoPage::onItemLongPressStarted(TodoItemWidget *item,
                                      const QPoint &globalPos) {
    if (m_addInlineActive || anyInlineEditing()) {
        return;
    }
    beginDrag(item, globalPos);
}

void TodoPage::onItemDragMoved(TodoItemWidget *item, const QPoint &globalPos) {
    if (!m_dragActive || item != m_dragItem) {
        return;
    }

    updateDrag(globalPos);
}

void TodoPage::onItemDragReleased(TodoItemWidget *item,
                                  const QPoint &globalPos) {
    if (!m_dragActive || item != m_dragItem) {
        return;
    }

    finishDrag(true, globalPos);
}

void TodoPage::addTodoItem(const QString &text, qint64 todoId) {
    auto *item = new TodoItemWidget(text, m_scrollContent);
    item->setProperty("todo_id", todoId);

    connect(item, &TodoItemWidget::doubleClicked, this,
            [this](TodoItemWidget *clickedItem) {
                if (clickedItem == nullptr || m_dragActive) {
                    return;
                }
                removeTodoItem(clickedItem);
            });
    connect(item, &TodoItemWidget::longPressStarted, this,
            &TodoPage::onItemLongPressStarted);
    connect(item, &TodoItemWidget::dragMoved, this, &TodoPage::onItemDragMoved);
    connect(item, &TodoItemWidget::dragReleased, this,
            &TodoPage::onItemDragReleased);

    m_items.push_back(item);
    rebuildListLayout(false);
}

void TodoPage::removeTodoItem(TodoItemWidget *item) {
    const int index = m_items.indexOf(item);
    if (index < 0) {
        return;
    }

    const qint64 todoId = item->property("todo_id").toLongLong();

    if (item->graphicsEffect() == nullptr) {
        auto *effect = new QGraphicsOpacityEffect(item);
        effect->setOpacity(1.0);
        item->setGraphicsEffect(effect);
    }

    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(item->graphicsEffect());

    auto *fadeAnim = new QPropertyAnimation(effect, "opacity", item);
    fadeAnim->setDuration(160);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);

    connect(fadeAnim, &QPropertyAnimation::finished, this,
            [this, item, todoId]() {
                const int currentIndex = m_items.indexOf(item);
                if (currentIndex < 0) {
                    return;
                }

                m_items.removeAt(currentIndex);
                m_listLayout->removeWidget(item);
                item->deleteLater();

                onTodoCompleted(todoId);
                rebuildListLayout(true);
            });

    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void TodoPage::beginAddInline() {
    if (m_addInlineActive || m_dragActive || anyInlineEditing()) {
        return;
    }

    m_addInlineActive = true;
    m_addArea->hide();
    m_addEditorRow->show();
    m_addLineEdit->clear();
    m_addLineEdit->setFocus();
}

void TodoPage::finishAddInline(bool confirm) {
    if (!m_addInlineActive) {
        return;
    }

    const QString text = m_addLineEdit->text().trimmed();
    if (confirm && !text.isEmpty()) {
        const qint64 todoId = DbManager::instance().insertTodo(text);
        if (todoId >= 0) {
            addTodoItem(text, todoId);
        }
    }

    m_addLineEdit->clear();
    m_addEditorRow->hide();
    m_addArea->show();
    m_addInlineActive = false;
}

bool TodoPage::anyInlineEditing() const {
    for (auto *item : m_items) {
        if (item != nullptr && item->isEditing()) {
            return true;
        }
    }
    return false;
}

void TodoPage::beginDrag(TodoItemWidget *item, const QPoint &globalPos) {
    if (m_dragActive) {
        return;
    }

    const int index = m_items.indexOf(item);
    if (index < 0) {
        return;
    }

    m_dragActive = true;
    m_dragItem = item;
    m_dragFromIndex = index;
    m_placeholderIndex = index;

    m_items.removeAt(index);

    m_dragStartGlobalPos = globalPos;
    m_dragOffsetInItem = item->mapFromGlobal(globalPos);
    m_lastDragGlobalPos = globalPos;

    const int dragWidth = item->width();
    m_dragProxy->setText(item->text());
    m_dragProxy->setFixedWidth(dragWidth);

    const QString wrapMode =
        ConfigManager::instance().getConfig()["todoWrapMode"].toString("force");
    int wrapFlags = Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop;
    if (wrapMode != "word") {
        wrapFlags |= Qt::TextWrapAnywhere;
    }

    QFontMetrics fm(m_dragProxy->font());
    const QRect textRect =
        fm.boundingRect(QRect(0, 0, qMax(1, dragWidth - 24), 100000),
                        wrapFlags, item->text());
    const int proxyHeight = qMax(kItemMinHeight, textRect.height() + 20);
    m_dragProxy->setFixedHeight(proxyHeight);
    m_dragProxy->show();
    m_dragProxy->raise();

    m_dragItem->hide();

    m_placeholderItem->setText(item->text());
    m_placeholderItem->setFixedWidth(item->width());
    m_placeholderItem->setMinimumHeight(0);
    m_placeholderItem->setMaximumHeight(0);
    m_placeholderItem->show();

    rebuildListLayout(false);
    animateGapHeight(0, m_dragItem->preferredItemHeight());

    if (!m_autoScrollTimer.isActive()) {
        m_autoScrollTimer.start();
    }

    updateDrag(globalPos);
}

void TodoPage::updateDrag(const QPoint &globalPos) {
    if (!m_dragActive) {
        return;
    }

    m_lastDragGlobalPos = globalPos;
    updateDragProxyPosition(globalPos);

    const QPoint viewPos = m_scrollArea->viewport()->mapFromGlobal(globalPos);
    const int contentY =
        viewPos.y() + m_scrollArea->verticalScrollBar()->value();

    const int draggedHeight = (m_dragItem != nullptr)
                                  ? m_dragItem->preferredItemHeight()
                                  : kItemMinHeight;
    const int draggedCenterY =
        contentY - m_dragOffsetInItem.y() + draggedHeight / 2;

    int nextIndex = calcInsertIndex(draggedCenterY);

    const QPoint dragStartViewPos =
        m_scrollArea->viewport()->mapFromGlobal(m_dragStartGlobalPos);
    const int dragStartCenterY = dragStartViewPos.y() +
                                 m_scrollArea->verticalScrollBar()->value() -
                                 m_dragOffsetInItem.y() + draggedHeight / 2;

    if (qAbs(draggedCenterY - dragStartCenterY) < kListSpacing) {
        nextIndex = m_dragFromIndex;
    }

    if (nextIndex == m_placeholderIndex) {
        return;
    }

    m_placeholderIndex = nextIndex;
    rebuildListLayout(true);
}

void TodoPage::endDrag(const QPoint &globalPos) { finishDrag(true, globalPos); }

void TodoPage::finishDrag(bool commit, const QPoint &globalPos) {
    if (!m_dragActive || m_dragItem == nullptr) {
        return;
    }

    Q_UNUSED(globalPos)

    if (commit) {
        const int insertIndex = qBound(0, m_placeholderIndex, m_items.size());
        m_items.insert(insertIndex, m_dragItem);
        persistCurrentOrder();
    } else {
        const int restoreIndex = qBound(0, m_dragFromIndex, m_items.size());
        m_items.insert(restoreIndex, m_dragItem);
    }

    m_dragItem->show();
    m_dragProxy->hide();

    animateGapHeight(m_placeholderItem->maximumHeight(), 0);

    m_dragActive = false;
    m_dragItem = nullptr;
    m_dragFromIndex = -1;
    m_placeholderIndex = -1;
    m_dragStartGlobalPos = QPoint();

    m_autoScrollTimer.stop();
    rebuildListLayout(true);
}

void TodoPage::cancelDrag() { finishDrag(false, m_lastDragGlobalPos); }

bool TodoPage::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_addLineEdit && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            finishAddInline(false);
            return true;
        }
    }

    if (!m_dragActive) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseMove: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        updateDrag(mouseEvent->globalPos());
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        finishDrag(true, mouseEvent->globalPos());
        break;
    }
    case QEvent::KeyPress: {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            cancelDrag();
            return true;
        }
        break;
    }
    case QEvent::WindowDeactivate:
    case QEvent::ApplicationDeactivate:
    case QEvent::FocusOut:
        cancelDrag();
        break;
    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

int TodoPage::calcInsertIndex(int contentY) const {
    int index = 0;
    for (auto *item : m_items) {
        const QRect g = item->geometry();
        const int centerY = g.top() + g.height() / 2;
        if (contentY < centerY) {
            return index;
        }
        ++index;
    }

    return m_items.size();
}

void TodoPage::rebuildListLayout(bool withAnimation) {
    QVector<QPair<QWidget *, QRect>> oldGeometries;
    if (withAnimation) {
        for (auto *item : m_items) {
            oldGeometries.append({item, item->geometry()});
        }
        if (m_dragActive) {
            oldGeometries.append(
                {m_placeholderItem, m_placeholderItem->geometry()});
        }
    }

    while (m_listLayout->count() > 0) {
        auto *layoutItem = m_listLayout->takeAt(0);
        delete layoutItem;
    }

    for (int i = 0; i < m_items.size(); ++i) {
        if (m_dragActive && i == m_placeholderIndex) {
            m_listLayout->addWidget(m_placeholderItem);
        }
        m_listLayout->addWidget(m_items[i]);
    }

    if (m_dragActive && m_placeholderIndex == m_items.size()) {
        m_listLayout->addWidget(m_placeholderItem);
    }

    m_listLayout->addWidget(m_addEditorRow);
    m_listLayout->addWidget(m_addArea);
    m_listLayout->addStretch();

    if (!withAnimation) {
        return;
    }

    m_scrollContent->layout()->activate();

    for (const auto &entry : oldGeometries) {
        QWidget *widget = entry.first;
        if (widget == nullptr || !widget->isVisible()) {
            continue;
        }

        const QRect oldRect = entry.second;
        const QRect newRect = widget->geometry();
        if (oldRect.topLeft() == newRect.topLeft()) {
            continue;
        }

        auto *anim = new QPropertyAnimation(widget, "pos", this);
        anim->setDuration(kMoveAnimMs);
        anim->setStartValue(oldRect.topLeft());
        anim->setEndValue(newRect.topLeft());
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void TodoPage::onTodoCompleted(qint64 id) {
    if (id < 0) {
        return;
    }

    DbManager::instance().markCompleted(id);
}

void TodoPage::persistCurrentOrder() {
    QVector<qint64> orderedIds;
    orderedIds.reserve(m_items.size());

    for (auto *item : m_items) {
        if (item == nullptr) {
            continue;
        }

        const qint64 id = item->property("todo_id").toLongLong();
        if (id >= 0) {
            orderedIds.push_back(id);
        }
    }

    DbManager::instance().updateTodoOrder(orderedIds);
}

void TodoPage::updateDragProxyPosition(const QPoint &globalPos) {
    if (!m_dragActive || m_dragProxy == nullptr) {
        return;
    }

    const QPoint viewportPos =
        m_scrollArea->viewport()->mapFromGlobal(globalPos);
    const int x = 0;
    const int y = viewportPos.y() - m_dragOffsetInItem.y();
    m_dragProxy->move(x, y);
    m_dragProxy->raise();
}

void TodoPage::animateGapHeight(int from, int to) {
    auto *anim =
        new QPropertyAnimation(m_placeholderItem, "maximumHeight", this);
    anim->setDuration(kGapAnimMs);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    if (to == 0) {
        connect(anim, &QPropertyAnimation::finished, this,
                [this]() { m_placeholderItem->hide(); });
    }
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
