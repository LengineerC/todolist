#include "widget.h"
#include "./ui_widget.h"
#include "config_manager.h"
#include "constants.h"
#include "route_page.h"
#include "todo_page.h"
#include "utils.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget), m_navGroup(nullptr),
      m_navLeftLayout(nullptr), m_hasRouteButton(false) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window |
                   Qt::CustomizeWindowHint);
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    resize(Config::width, Config::height);
    setMinimumSize(Config::width, Config::height);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->setupUi(this);
    setupRouter();

    registerPage("todo", "Todo", new TodoPage(this));
    registerPage("done", "Done", new RoutePage("Done", this));
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

    auto *placeholder1 = new QPushButton("t1", ui->navBar);
    auto *placeholder2 = new QPushButton("t2", ui->navBar);
    placeholder1->setFlat(true);
    placeholder2->setFlat(true);
    placeholder1->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: "
        "rgba(0,0,0,140); }"
        "QPushButton:hover { color: rgba(0,0,0,220); }");
    placeholder2->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: "
        "rgba(0,0,0,140); }"
        "QPushButton:hover { color: rgba(0,0,0,220); }");
    ui->navLayout->addWidget(placeholder1);
    ui->navLayout->addWidget(placeholder2);

    connect(m_navGroup,
            static_cast<void (QButtonGroup::*)(QAbstractButton *)>(
                &QButtonGroup::buttonClicked),
            this, &Widget::onNavButtonClicked);
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
                                  " font-weight: 800;"
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
        separator->setStyleSheet(QString("color: %1; padding: 0 2px; font-weight: 800;")
                                     .arg(Utils::colorToRgba(textColor, 200)));
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
        const RECT winRect = {0, 0, width(), height()};
        const long x = GET_X_LPARAM(msg->lParam) - frameGeometry().x();
        const long y = GET_Y_LPARAM(msg->lParam) - frameGeometry().y();

        const bool resizeW = minimumWidth() != maximumWidth();
        const bool resizeH = minimumHeight() != maximumHeight();

        if (resizeW && resizeH) {
            if (x < winRect.left + borderWidth && y < winRect.top + borderWidth) {
                *result = HTTOPLEFT;
                return true;
            }
            if (x >= winRect.right - borderWidth && y < winRect.top + borderWidth) {
                *result = HTTOPRIGHT;
                return true;
            }
            if (x < winRect.left + borderWidth && y >= winRect.bottom - borderWidth) {
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
    }
#endif

    return QWidget::nativeEvent(eventType, message, result);
}

void Widget::onNavButtonClicked(QAbstractButton *button) {
    if (!m_buttonToRoute.contains(button)) {
        return;
    }

    switchToPage(m_buttonToRoute.value(button));
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
