#include <QJsonDocument>
#include "errorresponse.h"

ErrorResponse::ErrorResponse() :
    code(0), message()
{

}

ErrorResponse::ErrorResponse(int code, QString message) :
    code(code), message(message)
{

}

ErrorResponse::ErrorResponse(const QJsonObject &obj)
{
    if (obj.contains("code"))
        this->code = obj["code"].toDouble();

    if (obj.contains("message"))
        this->message = obj["message"].toString();
}

ErrorResponse::ErrorResponse(const QJsonValue &value) :
    ErrorResponse(value.toObject())
{

}

QJsonObject ErrorResponse::toJson() const
{
    QJsonObject obj;

    obj["code"] = this->code;
    obj["message"] = this->message;

    return obj;
}

ErrorResponse ErrorResponse::fromJson(QJsonObject obj)
{
    return ErrorResponse(obj);
}

ErrorResponse::operator QJsonObject() const
{
    return this->toJson();
}

ErrorResponse::operator QString() const
{
    return QString::fromUtf8(QJsonDocument(this->toJson()).toJson()).simplified();
}

QDebug operator<<(QDebug debug, const ErrorResponse &t)
{
    debug.nospace() << "ErrorResponse(";
    debug.nospace() << qPrintable(t) << ")";
    return debug.space();
}
