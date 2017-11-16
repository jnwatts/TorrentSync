#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include "json-rpc.h"

class QWebSocket;

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QWebSocket *socket, QObject *parent = nullptr);
    ~Client(void);

    operator QString() const;

signals:
    void requestReceived(JsonRpc msg);
    void disconnected(void);

public slots:
    void send(JsonRpc msg);

private slots:
    void handleTextMessage(QString msg);

private:
    QWebSocket *_socket;
    QString _peerAddress;
    int _peerPort;
};

QDebug operator<< (QDebug debug, const Client &c);

#endif // CLIENT_H
