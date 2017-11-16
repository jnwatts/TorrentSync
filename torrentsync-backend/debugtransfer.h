#ifndef DEBUGTRANSFER_H
#define DEBUGTRANSFER_H

#include <QObject>
#include "task.h"

class DebugTransfer : public Task
{
    Q_OBJECT
public:
    explicit DebugTransfer(QString hash, QObject *parent = nullptr);
    QString getType(void) const;

    double progressRate;
    int pollRate;

    bool start(void);

private slots:
    void update(void);

};

#endif // DEBUGTRANSFER_H
