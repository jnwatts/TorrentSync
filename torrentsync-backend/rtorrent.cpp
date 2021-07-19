#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "maia/maiaXmlRpcClient.h"
#include "maia/maiaFault.h"
#include "debug.h"

#include "torrentservice.h"
#include "rtorrent.h"

RTorrent::RTorrent(const QJsonObject &params, QObject *parent) : TorrentService(params, parent)
{
    this->request.setUrl(QUrl(params["url"].toString()));
    this->request.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
}

RTorrent::~RTorrent()
{
}

void RTorrent::labels(std::function<void(QStringList)> success, std::function<void(ErrorResponse)> failure)
{
    Q_UNUSED(failure);
    success({"unlabeled"});
}

void RTorrent::torrents(std::function<void(TorrentHash)> success, std::function<void(ErrorResponse)> failure)
{
    QVariantList args;
    args
        << "main"
        << "d.hash="
        << "d.name="
        << "d.directory=" // save_path
        << "d.bytes_done=" // progress
        << "d.size_bytes=" // total_wanted
        << "d.timestamp.started="; // date_added

    this->invoke("d.multicall", args,
        [success](QVariant &arg, QNetworkReply* reply) {
            Q_UNUSED(reply);
            TorrentHash torrents;
            for (QVariant v : arg.toList()) {
                QVariantList l = v.toList();
                Torrent t;
                t.hash = l[0].toString();
                t.name = l[1].toString();
                t.savePath = l[2].toString();
                t.progress = l[3].toDouble();
                t.totalWanted = l[4].toDouble();
                t.timeAdded = l[5].toDouble();
                t.label = "unlabeled";
                torrents[t.hash] = t;
            }
            success(torrents);
        },
        [failure](int faultCode, const QString &faultString, QNetworkReply* reply) {
            Q_UNUSED(reply);
            if (failure)
                failure(ErrorResponse(faultCode, faultString));
        }
    );
}

QNetworkReply *RTorrent::invoke(QString method, QVariantList args,
    std::function<void(QVariant &arg, QNetworkReply* reply)> aresponse,
    std::function<void(int faultCode, const QString &faultString, QNetworkReply* reply)> fault)
{
    qCDebug(TORRENTSERVICE_IO) << "RTorrent <-" << qPrintable(method) << args;
    // Copied from MaiaXmlRpcClient::call() and adapted to use std::function instead of slots

    MaiaObject* call = new MaiaObject(this);
    if (aresponse)
        connect(call, &MaiaObject::aresponse, aresponse);
    if (fault)
        connect(call, &MaiaObject::fault, fault);

    QNetworkReply* reply = this->manager.post( this->request,
        call->prepareCall(method, args).toUtf8() );

    connect(reply, &QNetworkReply::finished, [reply, call]() {
        QString response;
        if(reply->error() != QNetworkReply::NoError) {
            MaiaFault fault(-32300, reply->errorString());
            response = fault.toString();
        } else {
            response = QString::fromUtf8(reply->readAll());
        }
        qCDebug(TORRENTSERVICE_IO) << "RTorrent ->" << qPrintable(response);
        call->parseResponse(response, reply);
        reply->deleteLater();
    });

    return reply;
}
