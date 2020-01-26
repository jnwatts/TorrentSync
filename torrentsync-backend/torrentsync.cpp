#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <deluge.h>

#include "debug.h"

#include "torrentsync.h"
#include "debugtransfer.h"
#include "transfer.h"

TorrentSync::TorrentSync(QObject *parent) : QObject(parent),
    _deluge(nullptr), _fetchingLabels(false), _fetchingTorrents(false),
    _server(nullptr)
{
    this->labels << "unlabeled";
}

TorrentSync::~TorrentSync(void)
{
    if (this->_deluge)
        delete this->_deluge;

    if (this->_server)
        delete this->_server;
}

void TorrentSync::init(QString path)
{
    QFile f(path);
    if (f.open(QFile::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        this->_config = doc.object();
        f.close();
    }
}

void TorrentSync::initDatabase(QString env, bool init)
{
    QJsonObject db_config = this->_config[env].toObject();
    this->_database.setConfig(db_config);
    if (init)
        this->_database.initialize();
}

void TorrentSync::initDeluge()
{
    QJsonObject obj = this->_config["deluge"].toObject();
    this->_deluge = new Deluge(obj["url"].toString(), obj["password"].toString());
    if (obj.contains("http_auth")) {
        obj = obj["http_auth"].toObject();
        this->_deluge->setHttpAuth(obj["user"].toString(), obj["pass"].toString());
    }
}

void TorrentSync::initServer()
{
    QJsonObject obj = this->_config["websocket"].toObject();
    this->_server = new Server(obj["address"].toString(), obj["port"].toInt());
    connect(this->_server, &Server::clientConnected, this, &TorrentSync::clientConnected);
}

void TorrentSync::updateDeluge()
{
    this->_mutex.lock();
    if (this->_fetchingLabels || this->_fetchingLabels) {
        if (!this->_updating.wait(&this->_mutex, 5 * 1000)) {
            qCWarning(TS, "Timed out waiting for previous update");
            return;
        }
    }
    this->_fetchingLabels = true;
    this->_fetchingTorrents = true;
    this->_mutex.unlock();

    auto updateRefreshing = [this](void) {
        QJsonObject coreUpdate;

        this->_mutex.lock();
        coreUpdate["refreshing"] = (this->_fetchingLabels || this->_fetchingTorrents);
        this->_server->notifyClients(JsonRpc::Notification("core.update", coreUpdate));
        this->_mutex.unlock();
    };
    updateRefreshing();

    this->_deluge->auth([this, updateRefreshing](bool complete) {
        if (complete) {
            this->_deluge->labels([this, updateRefreshing](QStringList labels) {
                this->labels = labels;
                this->_server->notifyClients(JsonRpc::Notification("labels.update", QJsonArray::fromStringList(this->labels)));
                this->_fetchingLabels = false;
                updateRefreshing();
            }, [this, updateRefreshing](DelugeError error) {
                Q_UNUSED(error);
                this->_fetchingLabels = false;
                updateRefreshing();
            });
            this->_deluge->torrents([this, updateRefreshing](TorrentHash torrents) {
                if (torrents.size() > 0)
                    this->torrents = torrents;

                QStringList hashes = this->torrents.keys();
                this->_tasks.filter(hashes);
                this->_database.filter(hashes);
                this->updateTasks(hashes);

                this->updateClientTorrents(this->_clientState);
                this->updateClientTasks(this->_clientState);

                this->_fetchingTorrents = false;
                updateRefreshing();
            }, [this, updateRefreshing](DelugeError error) {
                Q_UNUSED(error);
                this->_fetchingTorrents = false;
                updateRefreshing();
            });
        }
    });
}

void TorrentSync::initDebugTasks(void)
{
    QStringList hashes;
    {
        Torrent t;
        t.hash = "10";
        t.name = "DebugTransfer 10";
        t.timeAdded = 9999;
        hashes << t.hash;
        this->torrents[t.hash] = t;
    }


    {
        Torrent t;
        t.hash = "30";
        t.name = "DebugTransfer 30";
        hashes << t.hash;
        this->torrents[t.hash] = t;
    }

    this->updateTasks(hashes);
}

void TorrentSync::handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    // Skip client and deluge IO (mostly to avoid noise, and also passwords)
    if (context.category == CLIENT_IO().categoryName())
        return;
    if (context.category == DELUGE_IO().categoryName())
        return;

    QString type_str;
    switch (type) {
        case QtDebugMsg:
            type_str = "Debug";
        case QtInfoMsg:
            type_str = "Info";
        case QtWarningMsg:
            type_str = "Warning";
        case QtCriticalMsg:
            type_str = "Critical";
        case QtFatalMsg:
            type_str = "Fatal";
    }

    this->_server->notifyClients(JsonRpc::Notification("core.message", QJsonObject({
        {"type", type_str},
        {"context", QJsonObject({
                {"category", context.category},
                {"file", context.file},
                {"function", context.function},
                {"line", context.line},
                {"version", context.version},
        })},
        {"text", msg},
    })));
}

void TorrentSync::clientConnected(Client *client)
{
    qCDebug(CLIENT_IO) << "Connect:" << *client;
    connect(client, &Client::requestReceived, this, &TorrentSync::handleRequest);
    connect(client, &Client::disconnected, this, &TorrentSync::clientDisconnected);
    this->_clientState[client] = ClientState();
}

void TorrentSync::clientDisconnected()
{
    auto client = qobject_cast<Client*>(QObject::sender());
    qCDebug(CLIENT_IO) << "Disconnect:" << *client;
    this->_clientState.remove(client);
}

void TorrentSync::handleRequest(JsonRpc request)
{
    auto client = qobject_cast<Client*>(QObject::sender());
    JsonRpc response;
    QJsonArray params = request.params.toArray();
    bool parsed = false;
    bool methodFound = true;

    if (request.method == "core.refresh") {
        // No params
        this->updateDeluge();
        parsed = true;
    } else if (request.method == "core.get_state") {
        // Params: label
        if (params.size() > 0 && params[0].isString()) {
            response = this->handleGetState(client, params[0].toString());
            parsed = true;
        }
    } else if (request.method == "torrent.transfer") {
        // Params: hash
        if (params.size() > 0 && params[0].isString()) {
            response = this->handleTransfer(client, params[0].toString());
            parsed = true;
        }
    } else if (request.method == "torrent.abort") {
        // Params: hash
        if (params.size() > 0 && params[0].isString()) {
            response = this->handleAbort(client, params[0].toString());
            parsed = true;
        }
    } else {
        methodFound = false;
    }

    if (!methodFound)
        response = this->invalidMethod();
    else if (!parsed)
        response = this->invalidParams();

    if (request.type == JsonRpc::REQUEST) {
        response.id = request.id;
        client->send(response);
    }
}

void TorrentSync::taskUpdated(void)
{
    auto task = qobject_cast<Task*>(QObject::sender());
    QJsonObject tasks;
    QJsonObject obj = task->toJson();
    obj.remove("hash");
    tasks[task->hash] = obj;
    auto msg = JsonRpc::Notification("tasks.update", tasks);
    this->_server->notifyClients(msg);
}

JsonRpc TorrentSync::handleGetState(Client *client, QString label)
{
    ClientState &clientState = this->_clientState[client];
    QJsonObject state;

    if (label == "")
        label = "unlabeled";

    if (!this->labels.contains(label))
        label = this->labels.first();

    clientState.label = label;
    clientState.hasLabel = true;

    state["label"] = label;

    state["labels"] = QJsonArray::fromStringList(this->labels);

    ClientPtrStateHash clients;
    clients[client] = clientState;
    this->updateClientTorrents(clients);
    this->updateClientTasks(clients);

    return this->success(state);
}

JsonRpc TorrentSync::handleTransfer(Client *client, QString hash)
{
    Q_UNUSED(client);
    if (this->torrents.contains(hash)) {
        auto t = this->_tasks.get(hash);
        if (!t) {
            t = this->initTask(hash);
            if (!t)
                return this->notImplemented();
        }
        this->_tasks.enqueue(t);
        return this->success();
    } else {
        return this->hashNotFound();
    }
}

JsonRpc TorrentSync::handleAbort(Client *client, QString hash)
{
    Q_UNUSED(client);
    Task *t = this->_tasks.get(hash);
    if (t) {
        t->abort();
        return this->success();
    }
    return this->hashNotFound();
}

JsonRpc TorrentSync::success(QJsonValue result)
{

    return JsonRpc::SuccessResponse(0, result);
}

JsonRpc TorrentSync::error(int code, QString msg, QJsonValue data)
{
    return JsonRpc::ErrorResponse(0, code, msg, data);
}

JsonRpc TorrentSync::invalidMethod()
{
    return this->error(1, "Invalid method");
}

JsonRpc TorrentSync::invalidParams()
{
    return this->error(2, "Invalid params");
}

JsonRpc TorrentSync::notImplemented()
{
    return this->error(3, "Method not yet implemented");
}

JsonRpc TorrentSync::parseError()
{
    return this->error(4, "Failed to parse request");
}

JsonRpc TorrentSync::hashNotFound()
{
    return this->error(5, "Hash not found");
}

void TorrentSync::updateTasks(QStringList &hashes)
{
    TaskStateHash states = this->_database.getStates(hashes);
    for (auto iter = states.begin(); iter != states.end(); iter++) {
        const QString &hash = iter.key();
        Task *t = this->_tasks.get(hash);
        if (!t) {
            t = this->initTask(hash);
            switch (iter.value()) {
            case Task::INIT:
            case Task::QUEUED:
            case Task::RUNNING:
                t->state = Task::INIT;
                break;
            case Task::COMPLETE:
            case Task::FAILED:
                t->state = iter.value();
                break;
            }
        }
    }
}

void TorrentSync::updateClientTorrents(ClientPtrStateHash &clients)
{
    auto filter = [](QString &label, TorrentHash &torrents) -> QJsonObject {
        QJsonObject filtered;
        foreach (auto &t, torrents) {
            if (t.label == label) {
                QJsonObject obj = t.toJson();
                obj.remove("hash");
                filtered[t.hash] = obj;
            }
        }
        return filtered;
    };

    for (auto iter = clients.begin(); iter != clients.end(); iter++) {
        auto *client = iter.key();
        auto &state = iter.value();
        QString &label = state.label;
        QHash<QString, QJsonObject> cache;
        QJsonObject torrents;
        if (state.hasLabel) {
            if (cache.contains(label)) {
                torrents = cache[label];
            } else {
                torrents = cache[label] = filter(label, this->torrents);
            }
            client->send(JsonRpc::Notification("torrents.update", torrents));
        }
    }
}

void TorrentSync::updateClientTasks(ClientPtrStateHash &clients)
{
    QJsonObject tasks;
    foreach (auto t, this->_tasks.all()) {
        QJsonObject obj = t->toJson();
        obj.remove("hash");
        tasks[t->hash] = obj;
    }

    foreach (auto client, clients.keys())
        client->send(JsonRpc::Notification("tasks.update", tasks));
}

Task *TorrentSync::initTask(QString hash)
{
    Task *t = nullptr;

    if (this->torrents.contains(hash)) {
        if (hash.size() < 40) {
            auto dt = new DebugTransfer(hash);
            dt->progressRate = 100.0 / hash.toDouble() / (1000.0 / (double)dt->pollRate);
            t = dt;
        } else {
            auto tr = new Transfer(hash);
            auto config = this->_config["transfer"].toObject();
            tr->torrent = this->torrents[hash];
            tr->rsync = config["rsync"].toString();
            tr->host = config["host"].toString();
            tr->dest = config["dest"].toString();
            t = tr;
        }
        if (t) {
            connect(t, &Task::stateChanged, this, &TorrentSync::taskUpdated);
            connect(t, &Task::progressChanged, this, &TorrentSync::taskUpdated);
            connect(t, &Task::stateChanged, &this->_database, &Database::stateChanged);
            this->_tasks.insert(t);
        }
    }

    return t;
}
