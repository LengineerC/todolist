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
    QSqlQuery createQuery(m_db);
    if (!createQuery.exec(
            "CREATE TABLE IF NOT EXISTS todos ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "time TEXT NOT NULL,"
            "content TEXT NOT NULL,"
            "completed INTEGER NOT NULL DEFAULT 0 CHECK(completed IN (0, 1)),"
            "sort_order INTEGER NOT NULL DEFAULT 0"
            ")")) {
        qWarning() << "Failed to create todos table:"
                   << createQuery.lastError().text();
        return false;
    }

    QSqlQuery addColumnQuery(m_db);
    if (!addColumnQuery.exec(
            "ALTER TABLE todos ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0")) {
        const QString errorText = addColumnQuery.lastError().text().toLower();
        if (!errorText.contains("duplicate column name")) {
            qWarning() << "Failed to add sort_order column:"
                       << addColumnQuery.lastError().text();
            return false;
        }
    }

    QSqlQuery normalizeQuery(m_db);
    if (!normalizeQuery.exec(
            "UPDATE todos SET sort_order = id WHERE sort_order = 0")) {
        qWarning() << "Failed to normalize sort_order:"
                   << normalizeQuery.lastError().text();
        return false;
    }

    return true;
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
    if (!query.exec("SELECT id, time, content, completed, sort_order FROM todos WHERE "
                    "completed = 0 ORDER BY sort_order ASC, id ASC")) {
        qWarning() << "Failed to load todos:" << query.lastError().text();
        return todos;
    }

    while (query.next()) {
        TodoRecord record;
        record.id = query.value(0).toLongLong();
        record.time = query.value(1).toString();
        record.content = query.value(2).toString();
        record.completed = query.value(3).toInt() != 0;
        record.sortOrder = query.value(4).toInt();
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

    int maxSortOrder = 0;
    QSqlQuery maxOrderQuery(m_db);
    if (!maxOrderQuery.exec("SELECT COALESCE(MAX(sort_order), 0) FROM todos")) {
        qWarning() << "Failed to query max sort order:"
                   << maxOrderQuery.lastError().text();
        m_db.rollback();
        return -1;
    }
    if (maxOrderQuery.next()) {
        maxSortOrder = maxOrderQuery.value(0).toInt();
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO todos (time, content, completed, sort_order) VALUES (?, ?, 0, ?)");
    query.addBindValue(currentIsoTime());
    query.addBindValue(content);
    query.addBindValue(maxSortOrder + 1);

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

bool DbManager::updateTodoOrder(const QVector<qint64> &orderedIds) {
    if (!init()) {
        return false;
    }

    if (!m_db.transaction()) {
        qWarning() << "Failed to start reorder transaction:"
                   << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("UPDATE todos SET sort_order = ? WHERE id = ?");

    for (int i = 0; i < orderedIds.size(); ++i) {
        const qint64 id = orderedIds[i];
        if (id < 0) {
            continue;
        }

        query.bindValue(0, i + 1);
        query.bindValue(1, id);

        if (!query.exec()) {
            qWarning() << "Failed to update todo order:" << query.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        qWarning() << "Failed to commit reorder transaction:"
                   << m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}
