#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include "database.h"

Database::Database(QObject *parent) : QObject(parent)
{
}

void Database::open(QJsonObject config)
{
    this->_db = QSqlDatabase::addDatabase(config["qsql_driver"].toString().toUpper());
    this->_db.setUserName(config["username"].toString());
    this->_db.setPassword(config["password"].toString());
    this->_db.setDatabaseName(config["database"].toString());
    this->_db.setHostName(config["host"].toString());
    if (!this->_db.open())
        qFatal("Failed to open database: %s", qPrintable(this->_db.lastError().text()));
}

Task::State Database::getState(QString hash)
{
    auto query = QSqlQuery(this->db());
    query.prepare("SELECT state FROM tasks WHERE hash = :hash");
    query.bindValue(":hash", hash);
    if (this->exec(query)) {
        if (query.next())
           return (Task::State)query.value(0).toInt();
    }
    return Task::INIT;
}

TaskStateHash Database::getStates(QStringList hashes)
{
    auto query = QSqlQuery(this->db());
    QHash<QString, Task::State> states;
    query.prepare("SELECT hash, state FROM tasks WHERE hash IN (:hashes)");
    this->bindHashes(query, hashes);
    if (this->exec(query)) {
        while (query.next()) {
            states[query.value(0).toString()] = (Task::State)query.value(1).toInt();
        }
    }
    return states;
}

void Database::stateChanged(void)
{
    auto task = qobject_cast<Task*>(QObject::sender());
    this->setState(task->hash, task->state);
}

void Database::setState(QString hash, Task::State state)
{
    auto query = QSqlQuery(this->db());
    query.prepare("REPLACE INTO tasks (hash, state) VALUES (:hash, :state)");
    query.bindValue(":hash", hash);
    query.bindValue(":state", state);
    this->exec(query);
}

void Database::filter(QStringList &hashes)
{
    auto query = QSqlQuery(this->db());
    query.exec("DELETE FROM tasks WHERE hash NOT IN (:hashes)");
    this->bindHashes(query, hashes);
    this->exec(query);
}

bool Database::exec(QSqlQuery &query)
{
    bool result = query.exec();
    if (!result) {
        QStringList values;
        auto boundValues = query.boundValues();
        auto iter = boundValues.begin();
        while (iter != boundValues.end()) {
            values << iter.key() + "=" + iter.value().toString();
            iter++;
        }
        qWarning("Query failed: error=\"%s\", query=\"%s\", values={%s}",
                 qPrintable(query.lastError().text()),
                 qPrintable(query.lastQuery()),
                 qPrintable(values.join(", ")));
    }
    return result;
}

void Database::bindHashes(QSqlQuery &query, QStringList &hashes)
{
    QMap<QString, QString> placeHolders;
    foreach (auto &hash, hashes)
        placeHolders[":hash" + QString::number(placeHolders.size())] = hash;

    query.prepare(query.lastQuery().replace(":hashes", placeHolders.keys().join(',')));

    for (auto iter = placeHolders.begin(); iter != placeHolders.end(); iter++) {
        query.bindValue(iter.key(), iter.value());
    }
}

QSqlDatabase &Database::db()
{
    auto &db = this->_db;
    if (!db.isValid())
        db.open();
    return db;
}
