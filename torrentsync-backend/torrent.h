#ifndef TORRENT_H
#define TORRENT_H

#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QJsonObject>
#include <QHash>

class Torrent
{
public:
    Torrent();

    QString hash;
    QString name;
    QString savePath;
    double progress;
    QString label;
    double timeAdded;
    double totalWanted;

    QJsonObject toJson(void) const;
    static Torrent fromJson(QJsonObject obj);
    operator QString() const;
    operator QJsonObject() const;
};

typedef QHash<QString, Torrent> TorrentHash;

QDebug operator<< (QDebug debug, const Torrent &t);

#endif // TORRENT_H
