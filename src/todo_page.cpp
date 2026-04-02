#include "todo_page.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

TodoPage::TodoPage(QWidget *parent) : QWidget(parent) {
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QString("background: transparent;"));

    auto *scrollContent = new QWidget(scrollArea);
    auto *listLayout = new QVBoxLayout(scrollContent);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(8);

    for (int i = 1; i <= 20; ++i) {
        auto *row = new QFrame(scrollContent);
        row->setFrameShape(QFrame::NoFrame);
        row->setMinimumHeight(44);

        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 0, 12, 0);

        auto *label = new QLabel(
            QString("Todo item placeholder %1 sadadsadasdnsakdhsakjdhsakjdhk")
                .arg(i, 2, 10, QChar('0')),
            row);
        rowLayout->addWidget(label);

        listLayout->addWidget(row);
    }

    listLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    rootLayout->addWidget(scrollArea);
}
