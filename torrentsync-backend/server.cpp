#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include "debug.h"

#include "server.h"

Server::Server(QString address, int port, QObject *parent) : QObject(parent),
    _server(nullptr), _clients()
{
    this->_server = new QWebSocketServer("", QWebSocketServer::NonSecureMode);
    if (!this->_server->listen(QHostAddress(address), port)) {
        qFatal("Unable to listen on %s:%d", qPrintable(address), port);
        this->_server->deleteLater();
        this->_server = nullptr;
    } else {
        qCDebug(SERVER, "Listening on %s:%d", qPrintable(address), port);
    }

    connect(this->_server, &QWebSocketServer::newConnection, this, &Server::newConnection);
}

void Server::notifyClients(JsonRpc msg)
{
    foreach (auto c, this->_clients) {
        c->send(msg);
    }
}

void Server::newConnection(void)
{
    QWebSocket *socket = this->_server->nextPendingConnection();
    auto *client = new Client(socket, this);
    this->_clients << client;
    connect(client, &Client::disconnected, this, &Server::clientDisconnected);
    emit this->clientConnected(client);
}

void Server::clientDisconnected()
{
    auto *client = qobject_cast<Client *>(QObject::sender());
    if (this->_clients.contains(client)) {
        this->_clients.removeAll(client);
        client->deleteLater();
    }
}
