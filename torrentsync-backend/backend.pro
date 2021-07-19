#-------------------------------------------------
#
# Project created by QtCreator 2017-11-14T12:31:12
#
#-------------------------------------------------

QT       += core network websockets sql xml
CONFIG   += c++14

TARGET = torrentsync
TEMPLATE = app

SOURCES += main.cpp \
    debug.cpp \
    deluge.cpp \
    rtorrent.cpp \
    errorresponse.cpp \
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
    debug.h \
    deluge.h \
    rtorrent.h \
    errorresponse.h \
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
    torrentservice.h \
    tasks.h

FORMS    +=

LIBS += -L../build-libmaia -lmaia
INCLUDEPATH += ../libmaia

isEmpty(PREFIX) {
 PREFIX = /usr/local
}
target.path = $$PREFIX/bin
INSTALLS += target
