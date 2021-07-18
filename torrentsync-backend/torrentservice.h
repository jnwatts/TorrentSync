#pragma once

#include <QObject>
#include <QStringList>

#include "errorresponse.h"
#include "torrent.h"

class TorrentService : public QObject
{
	Q_OBJECT
public:
	explicit TorrentService(const QJsonObject &params, QObject *parent = 0) :
		QObject(parent)
	{
		Q_UNUSED(params);
	}
	~TorrentService(void) = default;

	virtual void labels(std::function<void(QStringList)> success, std::function<void(ErrorResponse)> failure = nullptr) = 0;
	virtual void torrents(std::function<void(TorrentHash)> success, std::function<void(ErrorResponse)> failure = nullptr) = 0;
};