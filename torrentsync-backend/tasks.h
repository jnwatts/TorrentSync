#ifndef TASKS_H
#define TASKS_H

#include <QObject>
#include <QVector>
#include <QHash>
#include "task.h"

class Tasks : public QObject
{
    Q_OBJECT
public:
    explicit Tasks(QObject *parent = nullptr);

    void insert(Task *t);
    void enqueue(Task *t);
    void filter(QStringList &hashes);
    void destroy(Task *t);

    Task *active(void);

    QVector<Task *> &queue(void) { return this->_queue; }
    Task *get(QString hash) { return this->_tasks[hash]; }
    TaskPtrHash &all(void) { return this->_tasks; }

private slots:
    void checkQueue(void);

private:
    TaskPtrHash _tasks;
    QVector<Task *> _queue;
};

#endif // TASKS_H
