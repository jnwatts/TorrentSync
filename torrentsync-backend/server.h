#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include "client.h"
#include "json-rpc.h"

class QWebSocketServer;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QString address, int port, QObject *parent = nullptr);
    const QList<Client *> &clients() { return this->_clients; }

signals:
    void clientConnected(Client *client);

public slots:
    void notifyClients(JsonRpc msg);

private slots:
    void newConnection(void);
    void clientDisconnected(void);

private:
    QWebSocketServer *_server;
    QList<Client *> _clients;
};

#endif // SERVER_H
