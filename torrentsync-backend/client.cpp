#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>

#include "client.h"

Client::Client(QWebSocket *socket, QObject *parent) : QObject(parent),
    _socket(socket)
{
    connect(socket, &QWebSocket::textMessageReceived, this, &Client::handleTextMessage);
    connect(socket, &QWebSocket::disconnected, this, &Client::disconnected);
    this->_peerAddress = this->_socket->peerAddress().toString();
    this->_peerPort = this->_socket->peerPort();
}

Client::~Client()
{
    this->_socket->close();
    this->_socket->deleteLater();
}

Client::operator QString() const
{
    return QString::asprintf("%s:%d", qPrintable(this->_peerAddress), this->_peerPort);
}

void Client::send(JsonRpc msg)
{
    qDebug() << *this << "<-" << msg;
    this->_socket->sendTextMessage((QString)msg);
}

void Client::handleTextMessage(QString msg)
{
    auto doc = QJsonDocument::fromJson(msg.toUtf8());
    if (doc.isObject()) {
        JsonRpc request = JsonRpc::fromJson(doc.object());
        if (request.type != JsonRpc::PARSE_ERROR) {
            qDebug() << *this << "->" << request;
            emit this->requestReceived(request);
        }
    } else
        qWarning("Received invalid message from client"); // TODO: Unified logging
}

QDebug operator<<(QDebug debug, const Client &c)
{
    debug.nospace() << "Client(";
    debug.nospace() << qPrintable(c) << ")";
    return debug.space();
}
