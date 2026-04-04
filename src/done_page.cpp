#include "done_page.h"

#include "config_manager.h"
#include "db_manager.h"
#include "utils.h"

#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace {
constexpr int kTopRowHeight = 46;
constexpr int kDateTitleHeight = 34;
constexpr int kItemRowHeight = 48;
constexpr int kRowSpacing = 8;
constexpr int kActionButtonSize = 30;

class ElidedLabel : public QLabel {
  public:
    explicit ElidedLabel(QWidget *parent = nullptr) : QLabel(parent) {}

    void setFullText(const QString &text) {
        m_fullText = text;
        updateElidedText();
    }

  protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        updateElidedText();
    }

  private:
    void updateElidedText() {
        setText(fontMetrics().elidedText(m_fullText, Qt::ElideRight,
                                         qMax(0, contentsRect().width())));
    }

    QString m_fullText;
};
} // namespace

DonePage::DonePage(QWidget *parent)
    : QWidget(parent), m_scrollArea(nullptr), m_scrollContent(nullptr),
      m_groupsLayout(nullptr), m_clearAllButton(nullptr) {
    setupUi();
    reloadData();
}

void DonePage::refreshData() { reloadData(); }

void DonePage::applyTheme() {
    const QString theme =
        ConfigManager::instance().getConfig()["theme"].toString("light");
    const QColor baseTextColor = Config::Themes::getTheme(theme).textColor;

    m_clearAllButton->setIcon(Utils::getColoredSvg(
        ":/icons/delete", QColor(baseTextColor.red(), baseTextColor.green(),
                                 baseTextColor.blue(), 100)));

    const QString hoverColor = Utils::colorToRgba(
        Config::Themes::getReverseTheme(theme).backgroundColor, 100);
    m_clearAllButton->setStyleSheet(QString("QPushButton {"
                                            " background: transparent;"
                                            " border-radius: 8px;"
                                            "}"
                                            "QPushButton:hover {"
                                            " background: %1;"
                                            " border-color: rgba(0, 0, 0, 130);"
                                            "}")
                                        .arg(hoverColor));

    reloadData();
    update();
}

void DonePage::setupUi() {
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    const QColor baseTextColor =
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor;
    const QString hoverColor = Utils::colorToRgba(
        Config::Themes::getReverseTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .backgroundColor,
        100);

    m_clearAllButton = new QPushButton(this);
    m_clearAllButton->setIconSize(QSize(28, 28));
    m_clearAllButton->setCursor(Qt::PointingHandCursor);
    m_clearAllButton->setFixedHeight(kTopRowHeight);
    m_clearAllButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_clearAllButton->setIcon(Utils::getColoredSvg(
        ":/icons/delete", QColor(baseTextColor.red(), baseTextColor.green(),
                                 baseTextColor.blue(), 100)));

    m_clearAllButton->setStyleSheet(QString("QPushButton {"
                                            " background: transparent;"
                                            " border-radius: 8px;"
                                            "}"
                                            "QPushButton:hover {"
                                            " background: %1;"
                                            " border-color: rgba(0, 0, 0, 130);"
                                            "}")
                                        .arg(hoverColor));
    rootLayout->addWidget(m_clearAllButton);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("background: transparent;");

    m_scrollContent = new QWidget(m_scrollArea);
    m_groupsLayout = new QVBoxLayout(m_scrollContent);
    m_groupsLayout->setContentsMargins(0, 0, 0, 0);
    m_groupsLayout->setSpacing(12);

    m_scrollArea->setWidget(m_scrollContent);
    rootLayout->addWidget(m_scrollArea, 1);

    connect(m_clearAllButton, &QPushButton::clicked, this, [this]() {
        if (DbManager::instance().clearCompletedTodos()) {
            reloadData();
        }
    });
}

QWidget *DonePage::buildDateGroup(const QString &dateText) {
    auto *group = new QWidget(m_scrollContent);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(6);

    auto *dateLabel = new QLabel(dateText, group);
    dateLabel->setFixedHeight(kDateTitleHeight);
    dateLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    QFont dateFont = dateLabel->font();
    dateFont.setPixelSize(Config::contentTextFontPx);
    dateLabel->setFont(dateFont);

    const QString textColor = Utils::colorToRgba(
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor,
        150);
    dateLabel->setStyleSheet(
        QString("font-weight: bold; color: %1;").arg(textColor));

    groupLayout->addWidget(dateLabel);
    return group;
}

QWidget *DonePage::buildDoneItemRow(qint64 id, const QString &content) {
    auto *row = new QWidget(m_scrollContent);
    row->setFixedHeight(kItemRowHeight);
    row->setMinimumWidth(0);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(12, 8, 12, 8);
    rowLayout->setSpacing(kRowSpacing);

    const QColor baseTextColor =
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor;
    const QString textColor = Utils::colorToRgba(baseTextColor, 255);
    const QString hoverColor = Utils::colorToRgba(
        Config::Themes::getReverseTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .backgroundColor,
        100);

    auto *contentLabel = new ElidedLabel(row);
    contentLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    contentLabel->setMinimumWidth(0);
    contentLabel->setFixedHeight(35);
    contentLabel->setWordWrap(false);
    QFont contentFont = contentLabel->font();
    contentFont.setPixelSize(Config::contentTextFontPx);
    contentLabel->setFont(contentFont);
    contentLabel->setStyleSheet(QString("color: %1;").arg(textColor));
    contentLabel->setFullText(content);

    auto *restoreButton = new QPushButton(row);
    restoreButton->setCursor(Qt::PointingHandCursor);
    restoreButton->setFixedSize(kActionButtonSize, kActionButtonSize);
    restoreButton->setIcon(Utils::getColoredSvg(
        ":/icons/undo", QColor(baseTextColor.red(), baseTextColor.green(),
                               baseTextColor.blue(), 100)));
    restoreButton->setStyleSheet(QString("QPushButton {"
                                         " background: transparent;"
                                         " border-radius: 4px;"
                                         " padding: 4px;"
                                         "}"
                                         "QPushButton:hover {"
                                         " background: %1;"
                                         "}")
                                     .arg(hoverColor));

    auto *deleteButton = new QPushButton(row);
    deleteButton->setCursor(Qt::PointingHandCursor);
    deleteButton->setFixedSize(kActionButtonSize, kActionButtonSize);
    deleteButton->setIcon(Utils::getColoredSvg(
        ":/icons/close", QColor(baseTextColor.red(), baseTextColor.green(),
                                baseTextColor.blue(), 100)));
    deleteButton->setStyleSheet(QString("QPushButton {"
                                        " background: transparent;"
                                        " border-radius: 4px;"
                                        " padding: 4px;"
                                        "}"
                                        "QPushButton:hover {"
                                        " background: %1;"
                                        "}")
                                    .arg(hoverColor));

    connect(restoreButton, &QPushButton::clicked, this,
            [this, row, id]() { removeDoneItemWithFade(row, id, true); });

    connect(deleteButton, &QPushButton::clicked, this,
            [this, row, id]() { removeDoneItemWithFade(row, id, false); });

    rowLayout->addWidget(contentLabel, 1);
    rowLayout->addWidget(restoreButton, 0, Qt::AlignCenter);
    rowLayout->addWidget(deleteButton, 0, Qt::AlignCenter);

    return row;
}

void DonePage::removeDoneItemWithFade(QWidget *row, qint64 id, bool restore) {
    if (row == nullptr || id < 0) {
        return;
    }

    if (row->graphicsEffect() == nullptr) {
        auto *effect = new QGraphicsOpacityEffect(row);
        effect->setOpacity(1.0);
        row->setGraphicsEffect(effect);
    }

    auto *effect =
        qobject_cast<QGraphicsOpacityEffect *>(row->graphicsEffect());
    if (effect == nullptr) {
        return;
    }

    auto *fadeAnim = new QPropertyAnimation(effect, "opacity", row);
    fadeAnim->setDuration(160);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);

    connect(fadeAnim, &QPropertyAnimation::finished, this,
            [this, id, restore]() {
                const bool ok = restore ? DbManager::instance().restoreTodo(id)
                                        : DbManager::instance().deleteTodo(id);
                if (ok) {
                    reloadData();
                }
            });

    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void DonePage::reloadData() {
    while (QLayoutItem *item = m_groupsLayout->takeAt(0)) {
        if (item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QVector<TodoRecord> completed =
        DbManager::instance().loadCompletedTodos();

    QHash<QString, QVector<TodoRecord>> groups;
    QVector<QString> dateOrder;

    for (const TodoRecord &record : completed) {
        const QDateTime dt = QDateTime::fromString(record.time, Qt::ISODate);
        const QString dateKey = dt.isValid() ? dt.date().toString("yyyy-MM-dd")
                                             : QStringLiteral("Unknown date");
        if (!groups.contains(dateKey)) {
            dateOrder.push_back(dateKey);
        }
        groups[dateKey].push_back(record);
    }

    for (const QString &dateKey : dateOrder) {
        QWidget *group = buildDateGroup(dateKey);
        auto *groupLayout = qobject_cast<QVBoxLayout *>(group->layout());
        const QVector<TodoRecord> dayItems = groups.value(dateKey);
        for (const TodoRecord &record : dayItems) {
            groupLayout->addWidget(buildDoneItemRow(record.id, record.content));
        }
        m_groupsLayout->addWidget(group);
    }

    m_groupsLayout->addStretch(1);
}
