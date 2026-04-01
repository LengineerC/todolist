#include "route_page.h"

#include <QLabel>
#include <QVBoxLayout>

RoutePage::RoutePage(const QString &text, QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *label = new QLabel(text, this);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
}
