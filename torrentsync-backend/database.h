#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QJsonObject>
#include "task.h"

class QSqlDatabase;

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);

    void open(QJsonObject config);

    Task::State getState(QString hash);
    TaskStateHash getStates(QStringList hashes);

public slots:
    void stateChanged(void);
    void setState(QString hash, Task::State state);
    void filter(QStringList &hashes);

private:
    bool exec(QSqlQuery &query);
    void bindHashes(QSqlQuery &query, QStringList &hashes);

    QSqlDatabase _db;
};

#endif // DATABASE_H
