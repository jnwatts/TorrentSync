#include <QMetaEnum>

#include "debug.h"

#include "task.h"

Task::Task(QString hash, QObject *parent) : QObject(parent),
    hash(hash), state(INIT), progress(0.0)
{
    connect(this, &Task::stateChanged, [this]() {
        qCDebug(TASK) << "stateChanged" << this->toJson();
    });
}

QJsonObject Task::toJson() const
{
    QJsonObject obj;

    obj["hash"] = this->hash;
    obj["type"] = this->getType();
    obj["progress"] = this->progress;
    obj["state"] = QMetaEnum::fromType<Task::State>().valueToKey(this->state);

    return obj;
}

bool Task::queued(void)
{
    if (this->state != RUNNING) {
        this->setState(QUEUED);
        return true;
    }
    return false;
}

bool Task::start(void)
{
    if (this->state == QUEUED) {
        this->setState(RUNNING);
        return true;
    }
    return false;
}

bool Task::abort(void)
{
    if (this->state == RUNNING || this->state == QUEUED) {
        this->setState(FAILED);
        return true;
    }
    return false;
}

void Task::setState(Task::State state)
{
    if (this->state != state) {
        this->state = state;
        emit this->stateChanged(this->state);
    }
}

void Task::setProgress(double progress)
{
    if (this->progress != progress) {
        this->progress = progress;
        emit this->progressChanged(progress);
    }
}

void Task::finish(bool complete)
{
    if (this->state == RUNNING)
        this->setState(complete ? COMPLETE : FAILED);
}
