#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>

#include "debug.h"

#include "database.h"

Database::Database(QObject *parent) : QObject(parent)
{
}

void Database::initialize(void)
{
    auto query = QSqlQuery(this->db());

    query.prepare("DROP TABLE IF EXISTS tasks");
    if (!this->exec(query))
        return;

    query.prepare("CREATE TABLE tasks("
            "`hash` varchar(40) NOT NULL,"
            "`state` varchar(40) NOT NULL,"
            "PRIMARY KEY (`hash`)"
            ") DEFAULT CHARSET=utf8");
    if (!this->exec(query))
        return;
}

void Database::setConfig(QJsonObject config)
{
    this->_config = config;
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
    query.prepare("DELETE FROM tasks WHERE hash NOT IN (:hashes)");
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
        qCWarning(DATABASE, "Query failed: error=\"%s\", query=\"%s\", values={%s}",
                 qPrintable(query.lastError().text()),
                 qPrintable(query.lastQuery()),
                 qPrintable(values.join(", ")));
    } else {
        qCDebug(DATABASE, "%s", qPrintable(query.lastQuery()));
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
    auto &config = this->_config;

    if (db.isOpen()) {
        auto query = QSqlQuery(db);
        if (!query.exec("SELECT 1;")) {
            query.clear();
            db.close();
            QString connectionName = db.connectionName();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName);
        }
    } else {
    }

    if (!db.isOpen()) {
        db = QSqlDatabase::addDatabase(config["qsql_driver"].toString().toUpper());
        db.setUserName(config["username"].toString());
        db.setPassword(config["password"].toString());
        db.setDatabaseName(config["database"].toString());
        db.setHostName(config["host"].toString());
        if (!db.open())
            qCCritical(DATABASE, "Failed to open database: %s", qPrintable(db.lastError().text()));
    }

    return db;
}
