#ifndef DELUGE_H
#define DELUGE_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QVariantList>
#include <functional>

#include "delugeerror.h"
#include "torrent.h"

class QNetworkAccessManager;

class Deluge : public QObject
{
    Q_OBJECT
public:
    explicit Deluge(QString url, QString password, QObject *parent = 0);
    ~Deluge(void);

    void setHttpAuth(QString user, QString pass) { this->http_user = user; this->http_pass = pass; }

    void auth(std::function<void(bool)> success, std::function<void(DelugeError)> failure = nullptr);
    void labels(std::function<void(QStringList)> success, std::function<void(DelugeError)> failure = nullptr);
    void torrents(std::function<void(TorrentHash)> success, std::function<void(DelugeError)> failure = nullptr);

signals:


public slots:

private:
    void invoke(QString method, QVariantList params, std::function<void (QJsonValue)> success = nullptr, std::function<void (DelugeError)> failure = nullptr);

    bool authenticated;
    int id;
    QString url;
    QString password;
    QString http_user;
    QString http_pass;
    QNetworkAccessManager *mgr;
};

#endif // DELUGE_H
