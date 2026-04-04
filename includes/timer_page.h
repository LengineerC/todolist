#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

class TimerPage : public QWidget {
    Q_OBJECT

  public:
    explicit TimerPage(QWidget *parent = nullptr);
    void applyTheme();

  private slots:
    void onStartPauseClicked();
    void onResetClicked();
    void onTick();
    void onTimeEditCommitted();

  private:
    void loadFromConfig();
    void persistToConfig() const;
    void setRunning(bool running);
    void updateDisplay();
    void updateButtons();

    static int clampSeconds(int totalSeconds);

    QLineEdit *m_minutesEdit;
    QLineEdit *m_secondsEdit;
    QLabel *m_colonLabel;
    QPushButton *m_startPauseButton;
    QPushButton *m_resetButton;

    QTimer m_tickTimer;
    bool m_running;
    int m_initialSeconds;
    int m_remainingSeconds;
};
