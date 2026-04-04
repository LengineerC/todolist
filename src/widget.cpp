#include "widget.h"
#include "./ui_widget.h"
#include "config_manager.h"
#include "constants.h"
#include "done_page.h"
#include "route_page.h"
#include "todo_page.h"
#include "utils.h"

#include <QEvent>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget), m_navGroup(nullptr),
      m_navLeftLayout(nullptr), m_themeSwitchBtn(nullptr),
      m_hasRouteButton(false), m_lastSavedSize(0, 0), m_lastSavedPos(0, 0) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window |
                   Qt::CustomizeWindowHint);
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setMinimumSize(Config::width, Config::height);

    m_resizeSaveTimer.setInterval(350);
    m_resizeSaveTimer.setSingleShot(true);
    connect(&m_resizeSaveTimer, &QTimer::timeout, this,
            &Widget::persistWindowGeometry);

    m_moveSaveTimer.setInterval(350);
    m_moveSaveTimer.setSingleShot(true);
    connect(&m_moveSaveTimer, &QTimer::timeout, this,
            &Widget::persistWindowGeometry);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->setupUi(this);

    const QJsonObject config = ConfigManager::instance().getConfig();
    const int restoredWidth =
        qMax(Config::width, config["windowWidth"].toInt(Config::width));
    const int restoredHeight =
        qMax(Config::height, config["windowHeight"].toInt(Config::height));
    resize(restoredWidth, restoredHeight);

    if (!config["windowX"].isNull() && !config["windowY"].isNull()) {
        move(config["windowX"].toInt(), config["windowY"].toInt());
    }

    m_lastSavedSize = size();
    m_lastSavedPos = pos();

    setupRouter();

    registerPage("todo", "Todo", new TodoPage(this));
    registerPage("done", "Done", new DonePage(this));
    registerPage("timer", "Timer", new RoutePage("Timer", this));

    switchToPage("todo");
}

void Widget::setupRouter() {
    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    auto *leftContainer = new QWidget(ui->navBar);
    m_navLeftLayout = new QHBoxLayout(leftContainer);
    m_navLeftLayout->setContentsMargins(0, 0, 0, 0);
    m_navLeftLayout->setSpacing(0);

    ui->navLayout->addWidget(leftContainer);
    ui->navLayout->addStretch(1);

    m_themeSwitchBtn = new QPushButton(ui->navBar);
    m_themeSwitchBtn->setFlat(true);
    ui->navLayout->addWidget(m_themeSwitchBtn);

    auto *lockBtn = new QPushButton(ui->navBar);
    lockBtn->setFlat(true);
    lockBtn->setProperty("navToolButton", true);
    ui->navLayout->addWidget(lockBtn);

    connect(m_themeSwitchBtn, &QPushButton::clicked, this, [this]() {
        QJsonObject config = ConfigManager::instance().getConfig();
        const QString currentTheme = config["theme"].toString("light");
        config["theme"] = (currentTheme == "light") ? "dark" : "light";
        ConfigManager::instance().writeConfigJson(config);
        applyTheme();
    });

    connect(m_navGroup,
            static_cast<void (QButtonGroup::*)(QAbstractButton *)>(
                &QButtonGroup::buttonClicked),
            this, &Widget::onNavButtonClicked);

    applyTheme();
}

void Widget::registerPage(const QString &routeKey, const QString &title,
                          QWidget *page) {
    if (routeKey.isEmpty() || page == nullptr ||
        m_routeToIndex.contains(routeKey)) {
        return;
    }

    int pageIndex = ui->stackPages->indexOf(page);
    if (pageIndex < 0) {
        pageIndex = ui->stackPages->addWidget(page);
    }

    m_routeToIndex.insert(routeKey, pageIndex);

    auto *button = new QPushButton(title, this);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    static QString heavyFamily;
    static bool heavyFontTried = false;
    if (!heavyFontTried) {
        heavyFontTried = true;
        const int heavyFontId =
            QFontDatabase::addApplicationFont(":/fonts/heavy_font");
        if (heavyFontId != -1) {
            const QStringList families =
                QFontDatabase::applicationFontFamilies(heavyFontId);
            if (!families.isEmpty()) {
                heavyFamily = families.first();
            }
        }
    }
    if (!heavyFamily.isEmpty()) {
        QFont navFont = button->font();
        navFont.setFamily(heavyFamily);
        button->setFont(navFont);
    }
    const QColor textColor =
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .textColor;
    button->setStyleSheet(QString("QPushButton {"
                                  " background: transparent;"
                                  " min-width: 77px;"
                                  " border: none;"
                                  //   " border: 1px solid #000;"
                                  " padding: 2px 0px;"
                                  " text-align: center;"
                                  " color: %1;"
                                  " font-size: 24px;"
                                  " font-weight: 900;"
                                  "}"
                                  "QPushButton:hover {"
                                  " color: %2;"
                                  "}"
                                  "QPushButton:checked {"
                                  " color: %2;"
                                  " font-size: 26px;"
                                  "}")
                              .arg(Utils::colorToRgba(textColor, 150),
                                   Utils::colorToRgba(textColor, 255)));

    if (m_hasRouteButton) {
        auto *separator = new QLabel("|", this);
        separator->setStyleSheet(
            QString("color: %1; padding: 0 2px; font-weight: 900;")
                .arg(Utils::colorToRgba(textColor, 200)));
        if (!heavyFamily.isEmpty()) {
            QFont sepFont = separator->font();
            sepFont.setFamily(heavyFamily);
            separator->setFont(sepFont);
        }
        m_navLeftLayout->addWidget(separator);
    }
    m_navLeftLayout->addWidget(button);
    m_hasRouteButton = true;

    m_navGroup->addButton(button);
    m_buttonToRoute.insert(button, routeKey);
}

bool Widget::switchToPage(const QString &routeKey) {
    if (!m_routeToIndex.contains(routeKey)) {
        return false;
    }

    const int index = m_routeToIndex.value(routeKey);
    ui->stackPages->setCurrentIndex(index);

    if (auto *todoPage =
            qobject_cast<TodoPage *>(ui->stackPages->widget(index))) {
        todoPage->refreshData();
    }
    if (auto *donePage =
            qobject_cast<DonePage *>(ui->stackPages->widget(index))) {
        donePage->refreshData();
    }

    for (auto it = m_buttonToRoute.constBegin();
         it != m_buttonToRoute.constEnd(); ++it) {
        it.key()->setChecked(it.value() == routeKey);
    }

    return true;
}

QString Widget::currentRoute() const {
    const int currentIndex = ui->stackPages->currentIndex();
    for (auto it = m_routeToIndex.constBegin(); it != m_routeToIndex.constEnd();
         ++it) {
        if (it.value() == currentIndex) {
            return it.key();
        }
    }

    return QString();
}

void Widget::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMaximized() || isFullScreen()) {
            showNormal();
        }
    }

    QWidget::changeEvent(event);
}

bool Widget::nativeEvent(const QByteArray &eventType, void *message,
                         long *result) {
#ifdef Q_OS_WIN
    Q_UNUSED(eventType)

    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCHITTEST) {
        const LONG borderWidth = 8;
        const LONG captionHeight = 44;
        const RECT winRect = {0, 0, width(), height()};
        const long x = GET_X_LPARAM(msg->lParam) - frameGeometry().x();
        const long y = GET_Y_LPARAM(msg->lParam) - frameGeometry().y();

        const bool resizeW = minimumWidth() != maximumWidth();
        const bool resizeH = minimumHeight() != maximumHeight();

        if (resizeW && resizeH) {
            if (x < winRect.left + borderWidth &&
                y < winRect.top + borderWidth) {
                *result = HTTOPLEFT;
                return true;
            }
            if (x >= winRect.right - borderWidth &&
                y < winRect.top + borderWidth) {
                *result = HTTOPRIGHT;
                return true;
            }
            if (x < winRect.left + borderWidth &&
                y >= winRect.bottom - borderWidth) {
                *result = HTBOTTOMLEFT;
                return true;
            }
            if (x >= winRect.right - borderWidth &&
                y >= winRect.bottom - borderWidth) {
                *result = HTBOTTOMRIGHT;
                return true;
            }
        }

        if (resizeW) {
            if (x < winRect.left + borderWidth) {
                *result = HTLEFT;
                return true;
            }
            if (x >= winRect.right - borderWidth) {
                *result = HTRIGHT;
                return true;
            }
        }

        if (resizeH) {
            if (y < winRect.top + borderWidth) {
                *result = HTTOP;
                return true;
            }
            if (y >= winRect.bottom - borderWidth) {
                *result = HTBOTTOM;
                return true;
            }
        }

        if (y >= winRect.top + borderWidth && y < winRect.top + captionHeight) {
            QWidget *hitWidget =
                childAt(static_cast<int>(x), static_cast<int>(y));
            if (hitWidget == nullptr ||
                (!qobject_cast<QAbstractButton *>(hitWidget) &&
                 !hitWidget->property("dragDisabled").toBool())) {
                *result = HTCAPTION;
                return true;
            }
        }
    }
#endif

    return QWidget::nativeEvent(eventType, message, result);
}

void Widget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    if (isMaximized() || isFullScreen()) {
        return;
    }

    m_resizeSaveTimer.start();
}

void Widget::moveEvent(QMoveEvent *event) {
    QWidget::moveEvent(event);

    if (isMaximized() || isFullScreen()) {
        return;
    }

    m_moveSaveTimer.start();
}

void Widget::persistWindowGeometry() {
    if (isMaximized() || isFullScreen()) {
        return;
    }

    if (size() == m_lastSavedSize && pos() == m_lastSavedPos) {
        return;
    }

    QJsonObject config = ConfigManager::instance().getConfig();
    config["windowWidth"] = size().width();
    config["windowHeight"] = size().height();
    config["windowX"] = pos().x();
    config["windowY"] = pos().y();
    ConfigManager::instance().writeConfigJson(config);

    m_lastSavedSize = size();
    m_lastSavedPos = pos();
}

void Widget::onNavButtonClicked(QAbstractButton *button) {
    if (!m_buttonToRoute.contains(button)) {
        return;
    }

    switchToPage(m_buttonToRoute.value(button));
}

void Widget::updateThemeSwitchButton() {
    if (m_themeSwitchBtn == nullptr) {
        return;
    }

    const QString theme =
        ConfigManager::instance().getConfig()["theme"].toString("light");
    QColor iconColor = Config::Themes::getTheme(theme).textColor;
    iconColor.setAlpha(100);

    m_themeSwitchBtn->setIcon(
        Utils::getColoredSvg(theme == "light" ? ":/icons/moon" : ":/icons/sun",
                             iconColor));
    m_themeSwitchBtn->setIconSize(QSize(32, 32));
}

void Widget::applyThemeToNavigation() {
    const QString theme =
        ConfigManager::instance().getConfig()["theme"].toString("light");
    const QColor textColor = Config::Themes::getTheme(theme).textColor;

    const QString normalColor = Utils::colorToRgba(textColor, 150);
    const QString activeColor = Utils::colorToRgba(textColor, 255);
    const QString separatorColor = Utils::colorToRgba(textColor, 200);
    const QString btnHoverColor = Utils::colorToRgba(
        Config::Themes::getReverseTheme(theme).backgroundColor, 100);

    if (m_themeSwitchBtn != nullptr) {
        m_themeSwitchBtn->setStyleSheet(
            QString("QPushButton { background: transparent; border: none; "
                    "border-radius: 3px; width: 40px; height: 40px; }"
                    "QPushButton:hover { background: %1; }")
                .arg(btnHoverColor));
    }

    const auto navButtons = m_navGroup->buttons();
    for (auto *button : navButtons) {
        button->setStyleSheet(QString("QPushButton {"
                                      " background: transparent;"
                                      " min-width: 77px;"
                                      " border: none;"
                                      " padding: 2px 0px;"
                                      " text-align: center;"
                                      " color: %1;"
                                      " font-size: 24px;"
                                      " font-weight: 900;"
                                      "}"
                                      "QPushButton:hover {"
                                      " color: %2;"
                                      "}"
                                      "QPushButton:checked {"
                                      " color: %2;"
                                      " font-size: 26px;"
                                      "}")
                                  .arg(normalColor, activeColor));
    }

    const auto separators = ui->navBar->findChildren<QLabel *>();
    for (auto *separator : separators) {
        if (separator != nullptr && separator->text() == "|") {
            separator->setStyleSheet(
                QString("color: %1; padding: 0 2px; font-weight: 900;")
                    .arg(separatorColor));
        }
    }

    const auto navToolButtons =
        ui->navBar->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *btn : navToolButtons) {
        if (btn != nullptr && btn->property("navToolButton").toBool()) {
            btn->setStyleSheet(
                QString("QPushButton { background: transparent; border: none; "
                        "border-radius: 3px; width: 40px; height: 40px; }"
                        "QPushButton:hover { background: %1; }")
                    .arg(btnHoverColor));
        }
    }
}

void Widget::applyTheme() {
    updateThemeSwitchButton();
    applyThemeToNavigation();

    for (int i = 0; i < ui->stackPages->count(); ++i) {
        QWidget *page = ui->stackPages->widget(i);
        if (auto *todoPage = qobject_cast<TodoPage *>(page)) {
            todoPage->applyTheme();
        }
        if (auto *donePage = qobject_cast<DonePage *>(page)) {
            donePage->applyTheme();
        }
    }

    update();
}

void Widget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(
        Config::Themes::getTheme(
            ConfigManager::instance().getConfig()["theme"].toString())
            .backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), Config::borderRadius, Config::borderRadius);
}

Widget::~Widget() { delete ui; }
