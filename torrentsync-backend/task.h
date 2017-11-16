#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include "json-rpc.h"

class Task : public QObject
{
    Q_OBJECT
public:
    enum State {
        INIT,
        QUEUED,
        RUNNING,
        COMPLETE,
        FAILED,
    };
    Q_ENUM(State)

    explicit Task(QString hash, QObject *parent = nullptr);

    virtual QString getType(void) const = 0;
    virtual QJsonObject toJson(void) const;

    operator QJsonValue() const { return this->toJson(); }

    virtual bool queued(void);
    virtual bool start(void);
    virtual bool abort(void);

    QString hash;
    Task::State state;
    double progress;

signals:
    void progressChanged(double progress);
    void stateChanged(Task::State state);

protected:
    void setState(Task::State state);
    void setProgress(double progress);
    void finish(bool complete);
};

typedef QHash<QString, Task::State> TaskStateHash;
typedef QHash<QString, Task *> TaskPtrHash;

Q_DECLARE_METATYPE(Task::State)

#endif // TASK_H
