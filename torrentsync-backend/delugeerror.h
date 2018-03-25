#ifndef DELUGEERROR_H
#define DELUGEERROR_H

#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QJsonObject>
#include <QHash>

class DelugeError
{
public:
    DelugeError();
    DelugeError(int code, QString message);
    DelugeError(const QJsonObject &value);
    DelugeError(const QJsonValue &value);

    int code;
    QString message;

    QJsonObject toJson(void) const;
    static DelugeError fromJson(QJsonObject obj);
    operator QString() const;
    operator QJsonObject() const;
};

QDebug operator<< (QDebug debug, const DelugeError &t);

#endif