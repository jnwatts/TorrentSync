#include <QProcess>

#include "debug.h"

#include "transfer.h"

Transfer::Transfer(QString hash, QObject *parent) : Task(hash, parent),
    _process(nullptr)
{
    this->_re = QRegExp("([0-9]+)%");
}

QString Transfer::getType() const
{
    return "transfer";
}

bool Transfer::start(void)
{
    if (Task::start()) {
        auto p = new QProcess();
        QStringList args;

        p->setProgram(this->rsync);

        args << "-r" << "--info=progress2";
        args << this->host + ":'" + this->torrent.savePath + "/" + this->torrent.name + "'";
        args << this->dest;
        p->setArguments(args);

        p->start();

        connect(p, SIGNAL(readyRead()), this, SLOT(outputReady()));
        connect(p, SIGNAL(finished(int)), this, SLOT(finished(int)));

        this->_process = p;
        return true;
    }
    return false;
}

bool Transfer::abort(void)
{
    if (Task::abort() && this->_process) {
        this->_process->kill();
        this->_process = nullptr;
        return true;
    }
    return false;
}

void Transfer::outputReady(void)
{
    auto line = QString(this->_process->readAllStandardOutput()).simplified();
    if (_re.indexIn(line) > -1)
        this->setProgress(_re.capturedTexts().at(1).toDouble());
}

void Transfer::finished(int exitCode)
{
    if (exitCode != 0)
        qCWarning(TRANSFER) << QString(this->_process->readAllStandardError()).simplified();
    this->finish(exitCode == 0);
}
