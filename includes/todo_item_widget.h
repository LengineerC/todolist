#pragma once

#include <QLabel>
#include <QPoint>
#include <QTimer>
#include <QWidget>

class TodoItemWidget : public QWidget {
    Q_OBJECT

  public:
    explicit TodoItemWidget(const QString &text, QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    void setTextHidden(bool hidden);
    int preferredItemHeight() const;

  signals:
    void doubleClicked(TodoItemWidget *item);
    void longPressStarted(TodoItemWidget *item, const QPoint &globalPos);
    void dragMoved(TodoItemWidget *item, const QPoint &globalPos);
    void dragReleased(TodoItemWidget *item, const QPoint &globalPos);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void updateDynamicHeight();

    QLabel *m_label;
    QTimer m_longPressTimer;
    bool m_pressing;
    bool m_longPressActive;
    QPoint m_pressPos;
};
