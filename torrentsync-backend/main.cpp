#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <signal.h>

#include "debug.h"

#include "torrentsync.h"
#include "types.h"

void register_sig_handler(void);
void handle_sig_int(int sig);
void handle_message(QtMsgType type, const QMessageLogContext &context, const QString &msg);

static QtMessageHandler g_default_message_handler = nullptr;
static TorrentSync *g_ts;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    register_types();

    QCommandLineOption config({"c", "config"}, "Read config from JSON <file>", "file", "backend.json");
    QCommandLineOption env({"e", "env"}, "Database environment to use", "env", "development");

    QCommandLineParser parser;
    parser.setApplicationDescription("TorrentSync backend server");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({config, env});
    parser.process(a);

    TorrentSync ts(&a);
    g_ts = &ts;

    ts.init(parser.value(config));
    ts.initDatabase(parser.value(env));
    ts.initDeluge();
    ts.initServer();
    ts.initDebugTasks();

    g_default_message_handler = qInstallMessageHandler(handle_message);

    register_sig_handler();

    QObject::connect(&a, &QCoreApplication::aboutToQuit, []() {
        qDebug("qApp about to quit...");
    });

    return a.exec();
}

void register_sig_handler(void)
{
    struct sigaction sigint;

    sigint.sa_handler = handle_sig_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = 0;
    sigint.sa_flags |= SA_RESTART;

    sigaction(SIGINT, &sigint, 0);
}

void handle_sig_int(int sig)
{
    Q_UNUSED(sig);
    qDebug("Signal received, shutting down...");
    qApp->quit();
}

void handle_message(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_default_message_handler)
        g_default_message_handler(type, context, msg);
    if (g_ts)
        g_ts->handleMessage(type, context, msg);
}