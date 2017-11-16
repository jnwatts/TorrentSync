#ifndef JSONRPC_H
#define JSONRPC_H

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

class JsonRpc {
public:
    enum Type {
        PARSE_ERROR,
        REQUEST,
        NOTIFICATION,
        SUCCESS_RESPONSE,
        ERROR_RESPONSE,
    };

    JsonRpc(Type type = PARSE_ERROR);

    operator QString() const;
    operator QJsonObject() const;

    static JsonRpc Request(int id, QString method, QJsonValue params);
    static JsonRpc Notification(QString method, QJsonValue params);
    static JsonRpc SuccessResponse(int id, QJsonValue result);
    static JsonRpc ErrorResponse(int id, int code, QString message, QJsonValue data);
    static JsonRpc ParseError(void);

    QJsonObject toJson(void) const;
    static JsonRpc fromJson(QJsonObject obj);

    Type type;
    int id;
    QString method;
    QJsonValue params;
    QJsonValue result;
    QJsonValue error;
};

Q_DECLARE_METATYPE(JsonRpc)
QDebug operator<< (QDebug debug, const JsonRpc &msg);

#endif // JSONRPC_H
