#include <QJsonDocument>
#include "torrent.h"

Torrent::Torrent() :
    hash(), name(), savePath(), progress(0), label("unlabeled"), timeAdded(0), totalWanted(0)
{

}

QJsonObject Torrent::toJson() const
{
    QJsonObject obj;

    obj["hash"] = this->hash;
    obj["name"] = this->name;
    obj["save_path"] = this->savePath;
    obj["progress"] = this->progress;
    obj["label"] = this->label;
    obj["time_added"] = this->timeAdded;
    obj["total_wanted"] = this->totalWanted;

    return obj;
}

Torrent Torrent::fromJson(QJsonObject obj)
{
    Torrent t;

    if (obj.contains("hash"))
        t.hash = obj["hash"].toString();

    if (obj.contains("name"))
        t.name = obj["name"].toString();

    if (obj.contains("save_path"))
        t.savePath = obj["save_path"].toString();

    if (obj.contains("progress"))
        t.progress = obj["progress"].toDouble();

    if (obj.contains("label")) {
        t.label = obj["label"].toString();
        if (t.label.size() == 0)
            t.label = "unlabeled";
    }

    if (obj.contains("time_added"))
        t.timeAdded = obj["time_added"].toDouble();

    if (obj.contains("total_wanted"))
        t.totalWanted = obj["total_wanted"].toDouble();

    return t;
}

Torrent::operator QJsonObject() const
{
    return this->toJson();
}

Torrent::operator QString() const
{
    return QString::fromUtf8(QJsonDocument(this->toJson()).toJson()).simplified();
}

QDebug operator<<(QDebug debug, const Torrent &t)
{
    debug.nospace() << "Torrent(";
    debug.nospace() << qPrintable(t) << ")";
    return debug.space();
}
