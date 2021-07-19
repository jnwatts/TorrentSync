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

#include "torrentservice.h"
#include "deluge.h"
#include "json-rpc.h"

Deluge::Deluge(const QJsonObject &params, QObject *parent) : TorrentService(params, parent),
     authenticated(false), id(1)
{
    this->mgr = new QNetworkAccessManager();
    this->url = params["url"].toString();
    this->password = params["password"].toString();

    if (params.contains("http_auth")) {
        QJsonObject http_auth = params["http_auth"].toObject();
        this->http_user = http_auth["user"].toString();
        this->http_pass = http_auth["pass"].toString();
    }

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

void Deluge::labels(std::function<void(QStringList)> success, std::function<void(ErrorResponse)> failure)
{
    if (!this->authenticated) {
        this->auth([this, success, failure](bool result) {
            if (result) {
                this->authenticated = true;
                this->labels(success, failure);
            }
        }, failure);
        return;
    }
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

void Deluge::torrents(std::function<void(TorrentHash)> success, std::function<void(ErrorResponse)> failure)
{
    if (!this->authenticated) {
        this->auth([this, success, failure](bool result) {
            if (result) {
                this->authenticated = true;
                this->torrents(success, failure);
            }
        }, failure);
        return;
    }
    auto escape_path = [](const QString &in) -> QString {
        QString out;
        for (const QChar c : in) {
            switch (c.unicode()) {
                case '\'':
                    out += "'?'";
                    break;
                default:
                    out += c;
                    break;
            }
        }
        return out;
    };
    this->invoke("core.get_torrents_status", {QVariantList({}), QVariantList({"name", "save_path", "progress", "label", "time_added", "total_wanted"})}, [escape_path, success](QJsonValue result) {
        auto json_torrents = result.toObject();
        TorrentHash torrents;
        foreach (auto k, json_torrents.keys()) {
            auto t = Torrent::fromJson(json_torrents[k].toObject());
            t.hash = k;
            t.savePath = t.savePath + "/'" + escape_path(t.name) + "'";
            torrents[k] = t;
        }
        success(torrents);
    }, failure);
}

void Deluge::auth(std::function<void (bool)> success, std::function<void(ErrorResponse)> failure)
{
    //TODO: Move this above to be in order...
    this->invoke("auth.login", {this->password}, [success](QJsonValue result) {
        success(result.toBool());
    }, failure);
}

void Deluge::invoke(QString method, QVariantList params, std::function<void (QJsonValue)> success, std::function<void (ErrorResponse)> failure)
{
    auto request = QNetworkRequest(this->url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("accept", "application/json");
    auto data = (QString)JsonRpc::Request(this->id++, method, QJsonArray::fromVariantList(params));
    qCDebug(TORRENTSERVICE_IO) << "Deluge <-" << qPrintable(data);
    auto reply = this->mgr->post(request, data.toUtf8());
    connect(reply, &QNetworkReply::finished, [reply, success, failure]() {
        auto data = reply->readAll();
        qCDebug(TORRENTSERVICE_IO) << "Deluge ->" << qPrintable(data);
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

    connect(reply, &QNetworkReply::sslErrors, [failure](const QList<QSslError> &errors) {
        for (auto e : errors)
            qCWarning(DELUGE) << "SSL error: " << e;
    });

    connect(reply, &QNetworkReply::errorOccurred, [failure](QNetworkReply::NetworkError error)
    {
        qCWarning(DELUGE) << "Network error:" << error;
        if (failure)
            failure({-1, "Network error"});
    });
}

