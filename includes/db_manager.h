#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <QtGlobal>

struct TodoRecord {
    qint64 id;
    QString time;
    QString content;
    bool completed;
};

class DbManager : public QObject {
    Q_OBJECT

  private:
    explicit DbManager(QObject *parent = nullptr);

    bool openDatabase();
    bool ensureTodosTable();
    QString currentIsoTime() const;

    QSqlDatabase m_db;
    bool m_initialized;

  public:
    static DbManager &instance();

    bool init();
    QVector<TodoRecord> loadActiveTodos();
    qint64 insertTodo(const QString &content);
    bool markCompleted(qint64 id);

    DbManager(const DbManager &) = delete;
    DbManager operator=(const DbManager &) = delete;
};
