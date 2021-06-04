#include "SysHandler.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

    //-- class SysHandler --//

bool SysHandler::mIsIntRequest = false;

SysHandler::SysHandler():
    mpPrevSigsegv(std::signal(SIGSEGV, signalHandler)),
    mpPrevSigfpe(std::signal(SIGFPE, signalHandler)),
    mpPrevSigill(std::signal(SIGILL, signalHandler)),
    mpPrevSigint(std::signal(SIGINT, signalHandler)),
    mpPrevTerm(std::set_terminate(terminateHandler))
{
}

SysHandler::~SysHandler()
{
    if(mpPrevSigsegv != SIG_ERR)
        std::signal(SIGSEGV, mpPrevSigsegv);
    if(mpPrevSigfpe != SIG_ERR)
        std::signal(SIGFPE, mpPrevSigfpe);
    if(mpPrevSigill != SIG_ERR)
        std::signal(SIGILL, mpPrevSigill);
    if(mpPrevSigint != SIG_ERR)
        std::signal(SIGINT, mpPrevSigill);
    std::set_terminate(mpPrevTerm);
}

void SysHandler::init()
{
    static SysHandler sysHandler;
}

void SysHandler::signalHandler(int signal)
{
    switch(signal) {
        case SIGSEGV:
            std::cerr << "Segmentation fault" << std::endl;
            break;
        case SIGFPE:
            std::cerr << "Invalid arithmetic operation" << std::endl;
            break;
        case SIGILL:
            std::cerr << "Illegal instruction" << std::endl;
            break;
        case SIGINT:
            std::cerr << "Interrupt requested" << std::endl;
            mIsIntRequest = true;
            break;
    }
    std::exit(EXIT_FAILURE);
}

void SysHandler::terminateHandler()
{
    try {
        throw;
    }
    catch(const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
    catch(...) {
        std::cerr << "Unknown error" << std::endl;
    }
    exit(EXIT_FAILURE);
}
