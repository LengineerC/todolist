#pragma once

#include <QPushButton>
#include <QScrollArea>
#include <QWidget>

class QGraphicsOpacityEffect;
class QVBoxLayout;

class DonePage : public QWidget {
    Q_OBJECT

  public:
    explicit DonePage(QWidget *parent = nullptr);
    void refreshData();

  private:
    void setupUi();
    void reloadData();
    QWidget *buildDateGroup(const QString &dateText);
    QWidget *buildDoneItemRow(qint64 id, const QString &content);
    void removeDoneItemWithFade(QWidget *row, qint64 id, bool restore);

    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QVBoxLayout *m_groupsLayout;
    QPushButton *m_clearAllButton;
};
