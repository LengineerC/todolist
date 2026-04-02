#pragma once

#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QLabel;
class QVBoxLayout;
class TodoItemWidget;

class TodoPage : public QWidget {
    Q_OBJECT

  public:
    explicit TodoPage(QWidget *parent = nullptr);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private slots:
    void onItemDoubleClicked(TodoItemWidget *item);
    void onItemLongPressStarted(TodoItemWidget *item, const QPoint &globalPos);
    void onItemDragMoved(TodoItemWidget *item, const QPoint &globalPos);
    void onItemDragReleased(TodoItemWidget *item, const QPoint &globalPos);

  private:
    void addTodoItem(const QString &text);
    void removeTodoItem(TodoItemWidget *item);
    void beginDrag(TodoItemWidget *item, const QPoint &globalPos);
    void updateDrag(const QPoint &globalPos);
    void endDrag(const QPoint &globalPos);
    void finishDrag(bool commit, const QPoint &globalPos);
    void cancelDrag();
    int calcInsertIndex(int contentY) const;
    void rebuildListLayout(bool withAnimation);
    void onTodoCompleted(const QString &text);
    void updateDragProxyPosition(const QPoint &globalPos);
    void animateGapHeight(int from, int to);

    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QVBoxLayout *m_listLayout;
    QPushButton *m_addArea;
    TodoItemWidget *m_placeholderItem;
    QLabel *m_dragProxy;
    QVector<TodoItemWidget *> m_items;

    bool m_dragActive;
    TodoItemWidget *m_dragItem;
    int m_dragFromIndex;
    int m_placeholderIndex;
    QPoint m_dragOffsetInItem;
    QPoint m_dragStartGlobalPos;
    QTimer m_autoScrollTimer;
    QPoint m_lastDragGlobalPos;
};
