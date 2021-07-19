#pragma once

#include <QObject>
#include <QList>
#include <QStringList>
#include <QVariantList>
#include <functional>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "torrentservice.h"
#include "maia/maiaXmlRpcClient.h"

class RTorrent : public TorrentService
{
    Q_OBJECT
public:
    explicit RTorrent(const QJsonObject &params, QObject *parent = 0);
    ~RTorrent(void);

    void labels(std::function<void(QStringList)> success, std::function<void(ErrorResponse)> failure = nullptr);
    void torrents(std::function<void(TorrentHash)> success, std::function<void(ErrorResponse)> failure = nullptr);


signals:


public slots:

private:
    QNetworkReply *invoke(QString method, QVariantList args,
        std::function<void(QVariant &arg, QNetworkReply* reply)> aresponse,
        std::function<void(int faultCode, const QString &faultString, QNetworkReply* reply)> fault);

    // MaiaXmlRpcClient *rpc;
    QNetworkAccessManager manager;
    QNetworkRequest request;
    QString url;
    QString password;
    QString http_user;
    QString http_pass;
};
