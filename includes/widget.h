#ifndef WIDGET_H
#define WIDGET_H

#include <QAbstractButton>
#include <QButtonGroup>
#include <QByteArray>
#include <QHBoxLayout>
#include <QMap>
#include <QMoveEvent>
#include <QPoint>
#include <QPushButton>
#include <QResizeEvent>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QWidget>

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
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long *result) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

  private slots:
    void onNavButtonClicked(QAbstractButton *button);

  private:
    void setupRouter();
    void persistWindowGeometry();
    void applyTheme();
    void updateThemeSwitchButton();
    void applyThemeToNavigation();

    Ui::Widget *ui;
    QButtonGroup *m_navGroup;
    QMap<QString, int> m_routeToIndex;
    QMap<QAbstractButton *, QString> m_buttonToRoute;
    QHBoxLayout *m_navLeftLayout;
    QPushButton *m_themeSwitchBtn;
    bool m_hasRouteButton;
    QTimer m_resizeSaveTimer;
    QTimer m_moveSaveTimer;
    QSize m_lastSavedSize;
    QPoint m_lastSavedPos;
};
#endif // WIDGET_H
