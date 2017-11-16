#include <QTimer>
#include "debugtransfer.h"

DebugTransfer::DebugTransfer(QString hash, QObject *parent) : Task(hash, parent),
    progressRate(10.0), pollRate(500)
{
}

QString DebugTransfer::getType(void) const
{
    return "debug";
}

bool DebugTransfer::start()
{
    if (Task::start()) {
        this->progress = 0;
        QTimer::singleShot(this->pollRate, this, &DebugTransfer::update);
        return true;
    }
    return false;
}

void DebugTransfer::update()
{
    if (this->state == RUNNING) {
        double progress = this->progress + this->progressRate;
        if (progress >= 100.0) {
            this->progress = 100.0;
            this->finish(true);
        } else {
            this->setProgress(progress);
            QTimer::singleShot(this->pollRate, this, &DebugTransfer::update);
        }
    }
}
