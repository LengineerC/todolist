#ifndef WIDGET_H
#define WIDGET_H

#include <QAction>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QByteArray>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QMap>
#include <QMenu>
#include <QMoveEvent>
#include <QPoint>
#include <QPushButton>
#include <QRect>
#include <QResizeEvent>
#include <QSize>
#include <QString>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

  public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void registerPage(const QString &routeKey, const QString &title,
                      QWidget *page);
    bool switchToPage(const QString &routeKey);
    QString currentRoute() const;

  protected:
    void paintEvent(QPaintEvent *event);
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message,
                     qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long *result) override;
#endif
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

  private slots:
    void onNavButtonClicked(QAbstractButton *button);
    void checkLockHover();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

  private:
    void setupRouter();
    void persistWindowGeometry();
    void applyTheme();
    void updateThemeSwitchButton();
    void applyThemeToNavigation();
    void toggleLockState();
    void setLocked(bool locked);
    QRect lockButtonRectInWidget() const;
    void setWindowClickThrough(bool clickThrough);
    void initTray();
    void hideToTray();
    void restoreFromTray();
    void quitFromTray();

    Ui::Widget *ui;
    QButtonGroup *m_navGroup;
    QMap<QString, int> m_routeToIndex;
    QMap<QAbstractButton *, QString> m_buttonToRoute;
    QHBoxLayout *m_navLeftLayout;
    QPushButton *m_themeSwitchBtn;
    QPushButton *m_lockBtn;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_quitAction;
    bool m_forceQuit;
    bool m_trayAvailable;
    bool m_isLocked;
    bool m_hasRouteButton;
    QTimer m_lockHoverTimer;
    QTimer m_resizeSaveTimer;
    QTimer m_moveSaveTimer;
    QSize m_lastSavedSize;
    QPoint m_lastSavedPos;
};
#endif // WIDGET_H
