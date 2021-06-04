#include "SysHandler.h"
#include "Params.h"
#include "Common.h"
#include "Daemon.h"
#include "Server.h"
#include "Msg.h"
#include <iostream>

int main(int argc, char *argv[])
{
    SysHandler::init();

    Daemon daemon;
    if(daemon.active()) {
        std::ios::sync_with_stdio(true);
        Msg::setTimestamps(true);
    }

    std::setlocale(LC_ALL, "");

    Msg() << "WDVC - Windows Desktop Video Capturer";
    Msg() << "Release 0.23, October 2017, AtWT";

    Params::init(argc, argv);

    if(daemon.active()) {
        Server server;
        server.run();
        return EXIT_SUCCESS;
    }

    switch(Params()->option) {
        case Option::start:
        {
            daemon.start(argc, argv);
            daemon.install(argc, argv);
            break;
        }
        case Option::remove:
        {
            daemon.kill();
            daemon.uninstall();
            break;
        }
        case Option::logfile:
        {
            Msg(FILELINE) << "Log file: \"" << getLogFullName() << "\"";
            break;
        }
        case Option::console:
        {
            Msg(FILELINE) << "Starting server, press Ctrl-C to terminate";
            Server server;
            server.run();
            break;
        }
        case Option::help:
        {
            Params()->showUsage();
            break;
        }
    }
    return EXIT_SUCCESS;
}
