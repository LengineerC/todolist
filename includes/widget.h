#ifndef WIDGET_H
#define WIDGET_H

#include <QAbstractButton>
#include <QButtonGroup>
#include <QByteArray>
#include <QHBoxLayout>
#include <QMap>
#include <QString>
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

  private slots:
    void onNavButtonClicked(QAbstractButton *button);

  private:
    void setupRouter();

    Ui::Widget *ui;
    QButtonGroup *m_navGroup;
    QMap<QString, int> m_routeToIndex;
    QMap<QAbstractButton *, QString> m_buttonToRoute;
    QHBoxLayout *m_navLeftLayout;
    bool m_hasRouteButton;
};
#endif // WIDGET_H
