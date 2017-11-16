#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkCookie>

#include "deluge.h"
#include "json-rpc.h"

Deluge::Deluge(QString url, QString password, QObject *parent) : QObject(parent),
     debug(false), authenticated(false), id(1), url(url), password(password)
{
    this->mgr = new QNetworkAccessManager();
    connect(mgr, &QNetworkAccessManager::authenticationRequired, [=](QNetworkReply *reply, QAuthenticator *authenticator) {
        Q_UNUSED(reply);
        authenticator->setUser(this->http_user);
        authenticator->setPassword(this->http_pass);
    });
}

Deluge::~Deluge()
{
    delete this->mgr;
}

void Deluge::labels(std::function<void(QStringList)> complete)
{
    this->invoke("label.get_labels", {}, [complete](QJsonObject response) {
        if (response.contains("result")) {
            auto json_labels = response["result"].toArray();
            QStringList labels;
            labels << "unlabeled";
            foreach (auto label, json_labels) {
                QString l = label.toString();
                labels << l;
            }
            complete(labels);
        } else {
            complete({});
        }
    });
}

void Deluge::torrents(std::function<void(TorrentHash)> complete)
{
    this->invoke("core.get_torrents_status", {QVariantList({}), QVariantList({"name", "save_path", "progress", "label", "time_added", "total_wanted"})}, [complete](QJsonObject response) {
        if (response.contains("result")) {
            auto json_torrents = response["result"].toObject();
            TorrentHash torrents;
            foreach (auto k, json_torrents.keys()) {
                auto t = Torrent::fromJson(json_torrents[k].toObject());
                t.hash = k;
                torrents[k] = t;
            }
            complete(torrents);

        } else {
            complete({});
        }
    });
}

void Deluge::auth(std::function<void (bool)> complete)
{
    this->invoke("auth.login", {this->password}, [complete](QJsonObject response) {
        if (response.contains("result")) {
            complete(response["result"].toBool());
        } else {
            complete(false);
        }
    });
}

void Deluge::invoke(QString method, QVariantList params, std::function<void (QJsonObject)> response)
{
    auto request = QNetworkRequest(this->url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("accept", "application/json");
    auto data = (QString)JsonRpc::Request(this->id++, method, QJsonArray::fromVariantList(params));
    if (this->debug)
        qDebug() << "Deluge <-" << qPrintable(data);
    auto reply = this->mgr->post(request, data.toUtf8());
    connect(reply, &QNetworkReply::finished, [this, reply, response]() {
        auto data = reply->readAll();
        if (this->debug)
            qDebug() << "Deluge ->" << qPrintable(data);
        auto obj = QJsonDocument::fromJson(data).object();
        if (response)
            response(obj);
    });
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, reply, response](QNetworkReply::NetworkError error) {
        qWarning() << "Error:" << error;
        response(QJsonObject());
    });
}

