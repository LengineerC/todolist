#include "timer_page.h"

#include "config_manager.h"
#include "constants.h"
#include "utils.h"

#include <QHBoxLayout>
#include <QIntValidator>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {
constexpr int kDefaultTimerSeconds = 25 * 60;
constexpr int kMaxMinutes = 999;
constexpr int kMaxSecondsPart = 60;
constexpr int kDisplayMaxSecondsPart = 59;
} // namespace

TimerPage::TimerPage(QWidget *parent)
    : QWidget(parent), m_minutesEdit(new QLineEdit(this)),
      m_secondsEdit(new QLineEdit(this)), m_colonLabel(new QLabel(":", this)),
      m_startPauseButton(new QPushButton(this)),
      m_resetButton(new QPushButton(this)), m_running(false),
      m_initialSeconds(kDefaultTimerSeconds),
      m_remainingSeconds(kDefaultTimerSeconds) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(34);

    root->addStretch(2);

    auto *timeContainer = new QWidget(this);
    auto *timeLayout = new QHBoxLayout(timeContainer);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(10);
    timeLayout->addStretch(1);
    timeLayout->addWidget(m_minutesEdit, 0, Qt::AlignCenter);
    timeLayout->addWidget(m_colonLabel, 0, Qt::AlignCenter);
    timeLayout->addWidget(m_secondsEdit, 0, Qt::AlignCenter);
    timeLayout->addStretch(1);
    root->addWidget(timeContainer);

    auto *buttonsContainer = new QWidget(this);
    auto *buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(20);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(m_startPauseButton, 0, Qt::AlignCenter);
    buttonsLayout->addWidget(m_resetButton, 0, Qt::AlignCenter);
    buttonsLayout->addStretch(1);
    root->addWidget(buttonsContainer);

    root->addStretch(2);

    QFont displayFont = m_minutesEdit->font();
    displayFont.setPointSize(52);
    displayFont.setWeight(QFont::Black);

    m_minutesEdit->setFont(displayFont);
    m_minutesEdit->setAlignment(Qt::AlignCenter);
    m_minutesEdit->setMaxLength(3);
    m_minutesEdit->setMinimumWidth(180);
    m_minutesEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_minutesEdit->setValidator(
        new QIntValidator(0, kMaxMinutes, m_minutesEdit));

    m_secondsEdit->setFont(displayFont);
    m_secondsEdit->setAlignment(Qt::AlignCenter);
    m_secondsEdit->setMaxLength(2);
    m_secondsEdit->setMinimumWidth(130);
    m_secondsEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_secondsEdit->setValidator(
        new QIntValidator(0, kMaxSecondsPart, m_secondsEdit));

    m_colonLabel->setFont(displayFont);
    m_colonLabel->setAlignment(Qt::AlignCenter);

    m_startPauseButton->setCursor(Qt::PointingHandCursor);
    m_resetButton->setCursor(Qt::PointingHandCursor);
    m_startPauseButton->setFixedHeight(44);
    m_resetButton->setFixedHeight(44);
    m_startPauseButton->setMinimumWidth(112);
    m_resetButton->setMinimumWidth(112);

    connect(m_startPauseButton, &QPushButton::clicked, this,
            &TimerPage::onStartPauseClicked);
    connect(m_resetButton, &QPushButton::clicked, this,
            &TimerPage::onResetClicked);
    connect(m_minutesEdit, &QLineEdit::editingFinished, this,
            &TimerPage::onTimeEditCommitted);
    connect(m_secondsEdit, &QLineEdit::editingFinished, this,
            &TimerPage::onTimeEditCommitted);

    m_tickTimer.setInterval(1000);
    connect(&m_tickTimer, &QTimer::timeout, this, &TimerPage::onTick);

    loadFromConfig();
    setRunning(false);
    updateDisplay();
    updateButtons();
    applyTheme();
}

void TimerPage::applyTheme() {
    const QString theme =
        ConfigManager::instance().getConfig()["theme"].toString("light");
    const QColor textColor = Config::Themes::getTheme(theme).textColor;
    const QColor reverseBg =
        Config::Themes::getReverseTheme(theme).backgroundColor;

    const QString inputTextColor = Utils::colorToRgba(textColor, 255);
    const QString inputBgColor =
        Utils::colorToRgba(Config::Themes::getTheme(theme).backgroundColor, 35);
    const QString inputBorderColor = Utils::colorToRgba(textColor, 90);
    const QString buttonBgColor = Utils::colorToRgba(textColor, 18);
    const QString buttonHoverColor = Utils::colorToRgba(reverseBg, 100);

    const QString inputStyle = QString("QLineEdit {"
                                       " color: %1;"
                                       " background: transparent;"
                                       // " border: 1px solid %3;"
                                       " border-radius: 12px;"
                                       // " padding: 12px 18px;"
                                       "}")
                                   .arg(inputTextColor);
    m_minutesEdit->setStyleSheet(inputStyle);
    m_secondsEdit->setStyleSheet(inputStyle);

    m_colonLabel->setStyleSheet(QString("color: %1;").arg(inputTextColor));

    const QString buttonStyle =
        QString("QPushButton {"
                " color: %1;"
                " background: %2;"
                " border: none;"
                " border-radius: 8px;"
                " padding: 8px 18px;"
                " font-size: 18px;"
                " font-weight: 700;"
                "}"
                "QPushButton:hover {"
                " background: %3;"
                "}")
            .arg(inputTextColor, buttonBgColor, buttonHoverColor);

    m_startPauseButton->setStyleSheet(buttonStyle);
    m_resetButton->setStyleSheet(buttonStyle);
}

void TimerPage::onStartPauseClicked() {
    if (m_running) {
        setRunning(false);
        return;
    }

    if (m_remainingSeconds <= 0) {
        m_remainingSeconds = m_initialSeconds;
        updateDisplay();
    }

    if (m_remainingSeconds <= 0) {
        return;
    }

    setRunning(true);
}

void TimerPage::onResetClicked() {
    setRunning(false);
    m_remainingSeconds = m_initialSeconds;
    updateDisplay();
}

void TimerPage::onTick() {
    if (!m_running) {
        return;
    }

    if (m_remainingSeconds > 0) {
        --m_remainingSeconds;
        updateDisplay();
    }

    if (m_remainingSeconds <= 0) {
        m_remainingSeconds = 0;
        setRunning(false);
        updateDisplay();
        QMessageBox::information(this, "Timer", "Time is up");
    }
}

void TimerPage::onTimeEditCommitted() {
    if (m_running) {
        updateDisplay();
        return;
    }

    bool minOk = false;
    bool secOk = false;

    int minutes = m_minutesEdit->text().trimmed().toInt(&minOk);
    int seconds = m_secondsEdit->text().trimmed().toInt(&secOk);

    if (!minOk) {
        minutes = 0;
    }
    if (!secOk) {
        seconds = 0;
    }

    minutes = qBound(0, minutes, kMaxMinutes);
    if (seconds > kMaxSecondsPart) {
        updateDisplay();
        return;
    }
    if (seconds == kMaxSecondsPart) {
        seconds = kDisplayMaxSecondsPart;
    }
    seconds = qBound(0, seconds, kDisplayMaxSecondsPart);

    m_initialSeconds = clampSeconds(minutes * 60 + seconds);
    m_remainingSeconds = m_initialSeconds;
    updateDisplay();
    persistToConfig();
}

void TimerPage::loadFromConfig() {
    const QJsonObject config = ConfigManager::instance().getConfig();
    m_initialSeconds =
        clampSeconds(config["timerInitialSeconds"].toInt(kDefaultTimerSeconds));
    m_remainingSeconds = m_initialSeconds;
}

void TimerPage::persistToConfig() const {
    QJsonObject config = ConfigManager::instance().getConfig();
    config["timerInitialSeconds"] = m_initialSeconds;
    ConfigManager::instance().writeConfigJson(config);
}

void TimerPage::setRunning(bool running) {
    m_running = running;
    if (m_running) {
        m_tickTimer.start();
    } else {
        m_tickTimer.stop();
    }

    m_minutesEdit->setReadOnly(m_running);
    m_secondsEdit->setReadOnly(m_running);
    updateButtons();
}

void TimerPage::updateDisplay() {
    const int safe = clampSeconds(m_remainingSeconds);
    const int minutes = safe / 60;
    const int seconds = safe % 60;

    m_minutesEdit->setText(QString("%1").arg(minutes, 2, 10, QLatin1Char('0')));
    m_secondsEdit->setText(QString("%1").arg(seconds, 2, 10, QLatin1Char('0')));
}

void TimerPage::updateButtons() {
    m_startPauseButton->setText(m_running ? "Pause" : "Start");
    m_resetButton->setText("Reset");
}

int TimerPage::clampSeconds(int totalSeconds) {
    if (totalSeconds < 0) {
        return 0;
    }

    const int maxTotal = kMaxMinutes * 60 + kDisplayMaxSecondsPart;
    return qMin(totalSeconds, maxTotal);
}
