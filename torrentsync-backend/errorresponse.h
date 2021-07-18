#ifndef DELUGEERROR_H
#define DELUGEERROR_H

#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QJsonObject>
#include <QHash>

class ErrorResponse
{
public:
    ErrorResponse();
    ErrorResponse(int code, QString message);
    ErrorResponse(const QJsonObject &value);
    ErrorResponse(const QJsonValue &value);

    int code;
    QString message;

    QJsonObject toJson(void) const;
    static ErrorResponse fromJson(QJsonObject obj);
    operator QString() const;
    operator QJsonObject() const;
};

QDebug operator<< (QDebug debug, const ErrorResponse &t);

#endif