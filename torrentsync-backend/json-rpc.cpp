#include <QJsonDocument>
#include <QDebug>
#include "json-rpc.h"

JsonRpc::JsonRpc(JsonRpc::Type type) :
    type(type), id(0), method(""), params(), result(), error()
{

}

JsonRpc::operator QJsonObject() const
{
    return this->toJson();
}

JsonRpc::operator QString() const
{
    return QString::fromUtf8(QJsonDocument(this->toJson()).toJson()).simplified();
}

JsonRpc JsonRpc::Request(int id, QString method, QJsonValue params)
{
    JsonRpc jrpc(REQUEST);
    jrpc.id = id;
    jrpc.method = method;
    jrpc.params = params;
    return jrpc;
}

JsonRpc JsonRpc::Notification(QString method, QJsonValue params)
{
    JsonRpc jrpc(NOTIFICATION);
    jrpc.method = method;
    jrpc.params = params;
    return jrpc;
}

JsonRpc JsonRpc::SuccessResponse(int id, QJsonValue result)
{
    JsonRpc jrpc(SUCCESS_RESPONSE);
    jrpc.id = id;
    jrpc.result = result;
    return jrpc;
}

JsonRpc JsonRpc::ErrorResponse(int id, int code, QString message, QJsonValue data)
{
    JsonRpc jrpc(ERROR_RESPONSE);
    QJsonObject error;
    jrpc.id = id;
    error["code"] = code;
    error["message"] = message;
    if (!data.isNull())
        error["data"] = data;
    jrpc.error = error;
    return jrpc;
}

JsonRpc JsonRpc::ParseError()
{
    JsonRpc jrpc(PARSE_ERROR);
    return jrpc;
}

QJsonObject JsonRpc::toJson() const
{
    QJsonObject obj = { { "jsonrpc", "2.0" } };

    if (this->type != NOTIFICATION)
        obj["id"] = this->id;

    switch (this->type) {
    case REQUEST:
    case NOTIFICATION:
        obj["method"] = this->method;
        obj["params"] = this->params;
        break;
    case SUCCESS_RESPONSE:
        obj["result"] = this->result;
        break;
    case ERROR_RESPONSE:
        obj["error"] = this->error;
        break;
    default:
        break;
    }

    return obj;
}

JsonRpc JsonRpc::fromJson(QJsonObject obj) {
    JsonRpc jrpc(PARSE_ERROR);

    if (!obj.contains("jsonrpc") && obj["jsonrpc"].toString() == "2.0")
        return jrpc;

    if (obj.contains("id")) {
        jrpc.id = obj["id"].toInt();

        if (obj.contains("method")) {
            jrpc.method = obj["method"].toString();
            if (obj.contains("params"))
                jrpc.params = obj["params"];
            jrpc.type = REQUEST;

        } else if (obj.contains("error")) {
            jrpc.error = obj["error"];
            jrpc.type = ERROR_RESPONSE;

        } else if (obj.contains("result")) {
            jrpc.error = obj["result"];
            jrpc.type = SUCCESS_RESPONSE;

        }
    } else {
        if (obj.contains("method"))
            jrpc.type = NOTIFICATION;
    }

    return jrpc;
}

QDebug operator<<(QDebug debug, const JsonRpc &msg)
{
    debug.nospace() << "JsonRpc(";
    debug.nospace() << qPrintable(msg) << ")";
    return debug.space();
}
