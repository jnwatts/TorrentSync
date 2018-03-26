#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkCookie>

#include "debug.h"

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

void Deluge::labels(std::function<void(QStringList)> success, std::function<void(DelugeError)> failure)
{
    this->invoke("label.get_labels", {}, [success](QJsonValue result) {
        auto json_labels = result.toArray();
        QStringList labels;
        labels << "unlabeled";
        foreach (auto label, json_labels) {
            QString l = label.toString();
            labels << l;
        }
        success(labels);
    }, failure);
}

void Deluge::torrents(std::function<void(TorrentHash)> success, std::function<void(DelugeError)> failure)
{
    this->invoke("core.get_torrents_status", {QVariantList({}), QVariantList({"name", "save_path", "progress", "label", "time_added", "total_wanted"})}, [success](QJsonValue result) {
        auto json_torrents = result.toObject();
        TorrentHash torrents;
        foreach (auto k, json_torrents.keys()) {
            auto t = Torrent::fromJson(json_torrents[k].toObject());
            t.hash = k;
            torrents[k] = t;
        }
        success(torrents);
    }, failure);
}

void Deluge::auth(std::function<void (bool)> success, std::function<void(DelugeError)> failure)
{
    //TODO: Move this above to be in order...
    this->invoke("auth.login", {this->password}, [success](QJsonValue result) {
        success(result.toBool());
    }, failure);
}

void Deluge::invoke(QString method, QVariantList params, std::function<void (QJsonValue)> success, std::function<void (DelugeError)> failure)
{
    auto request = QNetworkRequest(this->url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("accept", "application/json");
    auto data = (QString)JsonRpc::Request(this->id++, method, QJsonArray::fromVariantList(params));
    if (this->debug)
        qCDebug(DELUGE_IO) << "Deluge <-" << qPrintable(data);
    auto reply = this->mgr->post(request, data.toUtf8());
    connect(reply, &QNetworkReply::finished, [this, reply, success, failure]() {
        auto data = reply->readAll();
        if (this->debug)
            qCDebug(DELUGE_IO) << "Deluge ->" << qPrintable(data);
        auto obj = QJsonDocument::fromJson(data).object();
        QJsonValue result, error;

        if (obj.contains("error"))
            error = obj["error"];

        if (obj.contains("result"))
            result = obj["result"];

        if (error.isObject()) {
            qCWarning(DELUGE) << "Error:" << qPrintable(QString::fromUtf8(QJsonDocument(error.toObject()).toJson()).simplified());
            if (failure)
                failure(error);
        } else if (!result.isNull()) {
            if (success)
                success(result);
        } else {
            qCWarning(DELUGE) << "Error: response contains neither error nor result fields:" << qPrintable(QString::fromUtf8(QJsonDocument(obj).toJson()).simplified());
            if (failure)
                failure({-1, "Malformed response"});
        }

    });
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, reply, failure](QNetworkReply::NetworkError error) {
        qCWarning(DELUGE) << "Error:" << error;
        if (failure)
            failure({-1, "Network error"});
    });
}

