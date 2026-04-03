#include "db_manager.h"

#include "constants.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
const QString kConnectionName = "todolist_db_connection";
} // namespace

DbManager::DbManager(QObject *parent) : QObject(parent), m_initialized(false) {}

DbManager &DbManager::instance() {
    static DbManager s_instance;
    return s_instance;
}

bool DbManager::openDatabase() {
    if (m_db.isValid() && m_db.isOpen()) {
        return true;
    }

    if (QSqlDatabase::contains(kConnectionName)) {
        m_db = QSqlDatabase::database(kConnectionName);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    }

    m_db.setDatabaseName(Config::getDatabasePath());

    if (!m_db.open()) {
        qWarning() << "Failed to open sqlite database:"
                   << m_db.lastError().text();
        return false;
    }

    return true;
}

bool DbManager::ensureTodosTable() {
    QSqlQuery query(m_db);
    return query.exec(
        "CREATE TABLE IF NOT EXISTS todos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "time TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "completed INTEGER NOT NULL DEFAULT 0 CHECK(completed IN (0, 1))"
        ")");
}

QString DbManager::currentIsoTime() const {
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}

bool DbManager::init() {
    if (m_initialized) {
        return true;
    }

    if (!openDatabase()) {
        return false;
    }

    if (!ensureTodosTable()) {
        qWarning() << "Failed to ensure todos table:"
                   << m_db.lastError().text();
        return false;
    }

    m_initialized = true;
    return true;
}

QVector<TodoRecord> DbManager::loadActiveTodos() {
    QVector<TodoRecord> todos;
    if (!init()) {
        return todos;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT id, time, content, completed FROM todos WHERE "
                    "completed = 0 ORDER BY id ASC")) {
        qWarning() << "Failed to load todos:" << query.lastError().text();
        return todos;
    }

    while (query.next()) {
        TodoRecord record;
        record.id = query.value(0).toLongLong();
        record.time = query.value(1).toString();
        record.content = query.value(2).toString();
        record.completed = query.value(3).toInt() != 0;
        todos.push_back(record);
    }

    return todos;
}

qint64 DbManager::insertTodo(const QString &content) {
    if (!init()) {
        return -1;
    }

    if (!m_db.transaction()) {
        qWarning() << "Failed to start insert transaction:"
                   << m_db.lastError().text();
        return -1;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO todos (time, content, completed) VALUES (?, ?, 0)");
    query.addBindValue(currentIsoTime());
    query.addBindValue(content);

    if (!query.exec()) {
        qWarning() << "Failed to insert todo:" << query.lastError().text();
        m_db.rollback();
        return -1;
    }

    if (!m_db.commit()) {
        qWarning() << "Failed to commit insert transaction:"
                   << m_db.lastError().text();
        m_db.rollback();
        return -1;
    }

    return query.lastInsertId().toLongLong();
}

bool DbManager::markCompleted(qint64 id) {
    if (!init()) {
        return false;
    }

    if (!m_db.transaction()) {
        qWarning() << "Failed to start update transaction:"
                   << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("UPDATE todos SET completed = 1, time = ? WHERE id = ?");
    query.addBindValue(currentIsoTime());
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to mark todo completed:"
                   << query.lastError().text();
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        qWarning() << "Failed to commit update transaction:"
                   << m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}
