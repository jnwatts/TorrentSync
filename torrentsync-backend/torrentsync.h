#ifndef TORRENTSYNC_H
#define TORRENTSYNC_H

#include <QObject>
#include <QJsonObject>
#include <QWaitCondition>
#include <QMutex>
#include <QList>

#include "deluge.h"
#include "database.h"
#include "torrent.h"
#include "server.h"
#include "client.h"
#include "json-rpc.h"
#include "tasks.h"
#include "task.h"

class TorrentSync : public QObject
{
    Q_OBJECT
public:
    enum {
        NET_DEBUG       = 1ul<<0,
        CLIENT_DEBUG    = 1ul<<1,
    } DebugFlags;

    explicit TorrentSync(QObject *parent = nullptr);
    ~TorrentSync(void);

    void init(QString path);
    void initDatabase(QString env, bool init = false);
    void initDeluge(void);
    void initServer(void);
    void updateDeluge(void);
    void initDebugTasks(void);
    void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    JsonRpc getState();

    QStringList labels;
    TorrentHash torrents;

signals:

public slots:

private slots:
    void clientConnected(Client *client);
    void clientDisconnected(void);
    void handleRequest(JsonRpc request);
    void taskUpdated(void);

private:
    class ClientState {
    public:
        ClientState() : hasLabel(false), label()
        { }
        bool hasLabel;
        QString label;
    };
    typedef QHash<Client *, ClientState> ClientPtrStateHash;

    JsonRpc handleGetState(Client *client, QString label);
    JsonRpc handleTransfer(Client *client, QString hash);
    JsonRpc handleAbort(Client *client, QString hash);
    JsonRpc success(QJsonValue result = QJsonValue());
    JsonRpc error(int code, QString msg, QJsonValue data = QJsonValue());
    JsonRpc invalidMethod(void);
    JsonRpc invalidParams(void);
    JsonRpc notImplemented(void);
    JsonRpc parseError(void);
    JsonRpc hashNotFound(void);

    void updateTasks(QStringList &hashes);
    void updateClientTorrents(ClientPtrStateHash &clients);
    void updateClientTasks(ClientPtrStateHash &clients);
    Task *initTask(QString hash);

    QJsonObject _config;
    QMutex _mutex;
    QWaitCondition _updating;
    Deluge *_deluge;
    bool _fetchingLabels;
    bool _fetchingTorrents;
    Server *_server;
    ClientPtrStateHash _clientState;
    Tasks _tasks;
    Database _database;
};

#endif // TORRENTSYNC_H
