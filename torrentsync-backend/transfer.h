#ifndef TRANSFER_H
#define TRANSFER_H

#include <QObject>
#include <QProcess>
#include <QRegExp>
#include "task.h"
#include "torrent.h"

class Transfer : public Task
{
    Q_OBJECT
public:
    explicit Transfer(QString hash, QObject *parent = nullptr);
    QString getType(void) const;

    Torrent torrent;
    QString rsync;
    QString host;
    QString dest;

    bool start(void);
    bool abort(void);

private slots:
    void outputReady(void);
    void finished(int exitCode);
    void errorOccurred(QProcess::ProcessError error);

private:
    QProcess *_process;
    QRegExp _re;
};

#endif // TRANSFER_H
