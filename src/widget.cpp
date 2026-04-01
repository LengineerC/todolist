#include "widget.h"
#include "./ui_widget.h"
#include "constants.h"
#include "route_page.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget), m_navGroup(nullptr),
      m_navLeftLayout(nullptr), m_hasRouteButton(false) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window |
                   Qt::CustomizeWindowHint);
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setFixedSize(Config::width, Config::height);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->setupUi(this);
    setupRouter();

    registerPage("todo", "Todo", new RoutePage("Todo", this));
    registerPage("history", "History", new RoutePage("History", this));
    registerPage("pomodoro", "Pomodoro", new RoutePage("Pomodoro", this));

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

    auto *placeholder1 = new QPushButton("Placeholder", ui->navBar);
    auto *placeholder2 = new QPushButton("Placeholder", ui->navBar);
    placeholder1->setFlat(true);
    placeholder2->setFlat(true);
    placeholder1->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: rgba(0,0,0,140); }"
        "QPushButton:hover { color: rgba(0,0,0,220); }"
    );
    placeholder2->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: rgba(0,0,0,140); }"
        "QPushButton:hover { color: rgba(0,0,0,220); }"
    );
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
    button->setStyleSheet(
        "QPushButton {"
        " background: transparent;"
        " border: none;"
        " padding: 4px 10px;"
        " color: rgba(0, 0, 0, 140);"
        " font-size: 14px;"
        "}"
        "QPushButton:hover {"
        " color: rgba(0, 0, 0, 255);"
        "}"
        "QPushButton:checked {"
        " color: rgba(0, 0, 0, 255);"
        " font-size: 16px;"
        " font-weight: 600;"
        "}"
    );

    if (m_hasRouteButton) {
        auto *separator = new QLabel("|", this);
        separator->setStyleSheet("color: rgba(0, 0, 0, 100); padding: 0 6px;");
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
    painter.setBrush(Config::Themes::light.backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), Config::borderRadius, Config::borderRadius);
}

Widget::~Widget() { delete ui; }
