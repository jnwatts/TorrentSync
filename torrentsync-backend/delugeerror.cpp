#include <QJsonDocument>
#include "delugeerror.h"

DelugeError::DelugeError() :
    code(0), message()
{

}

DelugeError::DelugeError(int code, QString message) :
    code(code), message(message)
{

}

DelugeError::DelugeError(const QJsonObject &obj)
{
    if (obj.contains("code"))
        this->code = obj["code"].toDouble();

    if (obj.contains("message"))
        this->message = obj["message"].toString();
}

DelugeError::DelugeError(const QJsonValue &value) :
    DelugeError(value.toObject())
{

}

QJsonObject DelugeError::toJson() const
{
    QJsonObject obj;

    obj["code"] = this->code;
    obj["message"] = this->message;

    return obj;
}

DelugeError DelugeError::fromJson(QJsonObject obj)
{
    return DelugeError(obj);
}

DelugeError::operator QJsonObject() const
{
    return this->toJson();
}

DelugeError::operator QString() const
{
    return QString::fromUtf8(QJsonDocument(this->toJson()).toJson()).simplified();
}

QDebug operator<<(QDebug debug, const DelugeError &t)
{
    debug.nospace() << "DelugeError(";
    debug.nospace() << qPrintable(t) << ")";
    return debug.space();
}
