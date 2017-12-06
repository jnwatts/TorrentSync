#include <QTimer>
#include "tasks.h"
#include "task.h"

Tasks::Tasks(QObject *parent) : QObject(parent)
{

}

void Tasks::insert(Task *t)
{
    if (!this->_tasks[t->hash]) {
        this->_tasks[t->hash] = t;
        t->setParent(this);
        connect(t, &Task::progressChanged, this, &Tasks::checkQueue);
        connect(t, &Task::stateChanged, this, &Tasks::checkQueue);
    }
}

void Tasks::enqueue(Task *t)
{
    this->insert(t);
    this->_queue.append(t);
    t->queued();
    QTimer::singleShot(0, this, &Tasks::checkQueue);
}

void Tasks::filter(QStringList &hashes)
{
    QMutableHashIterator<QString, Task*> iter(this->_tasks);
    while (iter.hasNext()) {
        auto i = iter.next();
        auto &h = i.key();
        Task *t = i.value();
        if (t != nullptr && !hashes.contains(h)) {
            iter.remove();
            this->_queue.removeAll(t);
            t->abort();
            t->deleteLater();
        }
    }
    QTimer::singleShot(0, this, &Tasks::checkQueue);
}

void Tasks::destroy(Task *t)
{
    this->_tasks.remove(t->hash);
    this->_queue.removeAll(t);
    t->abort();
    t->deleteLater();
    QTimer::singleShot(0, this, &Tasks::checkQueue);
}

Task *Tasks::active(void)
{
    if (!this->_queue.isEmpty())
        return this->_queue.first();
    return nullptr;
}

void Tasks::checkQueue(void)
{
    bool running = false;
    QMutableVectorIterator<Task*> iter(this->_queue);
    while (iter.hasNext()) {
        Task *t = iter.next();
        switch (t->state) {
        case Task::INIT:
        case Task::QUEUED:
            if (!running) {
                t->start();
                running = true;
            }
            break;
        case Task::RUNNING:
            running = true;
            break;
        case Task::FAILED:
        case Task::COMPLETE:
            iter.remove();
            break;
        default:
            break;
        }
    }
}
