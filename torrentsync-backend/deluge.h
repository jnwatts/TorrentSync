#ifndef DELUGE_H
#define DELUGE_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QVariantList>
#include <functional>

#include "torrentservice.h"

class QNetworkAccessManager;

class Deluge : public TorrentService
{
    Q_OBJECT
public:
    explicit Deluge(const QJsonObject &params, QObject *parent = 0);
    ~Deluge(void);

    void labels(std::function<void(QStringList)> success, std::function<void(ErrorResponse)> failure = nullptr);
    void torrents(std::function<void(TorrentHash)> success, std::function<void(ErrorResponse)> failure = nullptr);

signals:


public slots:

private:
    void auth(std::function<void(bool)> success, std::function<void(ErrorResponse)> failure = nullptr);
    void invoke(QString method, QVariantList params, std::function<void (QJsonValue)> success = nullptr, std::function<void (ErrorResponse)> failure = nullptr);

    bool authenticated;
    int id;
    QString url;
    QString password;
    QString http_user;
    QString http_pass;
    QNetworkAccessManager *mgr;
};

#endif // DELUGE_H
