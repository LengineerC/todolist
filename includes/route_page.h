#ifndef ROUTE_PAGE_H
#define ROUTE_PAGE_H

#include <QString>
#include <QWidget>

class RoutePage : public QWidget {
    Q_OBJECT

  public:
    explicit RoutePage(const QString &text, QWidget *parent = nullptr);
};

#endif // ROUTE_PAGE_H
