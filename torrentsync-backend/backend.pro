#-------------------------------------------------
#
# Project created by QtCreator 2017-11-14T12:31:12
#
#-------------------------------------------------

QT       += core network websockets sql
CONFIG   += c++14

TARGET = torrentsync-backend
TEMPLATE = app


SOURCES += main.cpp \
    deluge.cpp \
    torrent.cpp \
    torrentsync.cpp \
    client.cpp \
    json-rpc.cpp \
    server.cpp \
    types.cpp \
    transfer.cpp \
    task.cpp \
    debugtransfer.cpp \
    database.cpp \
    tasks.cpp

HEADERS  += \
    deluge.h \
    torrent.h \
    torrentsync.h \
    client.h \
    json-rpc.h \
    server.h \
    types.h \
    transfer.h \
    task.h \
    debugtransfer.h \
    database.h \
    tasks.h

FORMS    +=
