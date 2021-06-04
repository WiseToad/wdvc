#include "Params.h"
#include "Msg.h"
#include <iostream>
#include <x264.h>

namespace {

enum Switch {
    capturer = 1, fps = 2, scale = 4, preset = 8, bitrate = 16,
    crf = 32, keyint = 64, intraRefresh = 128, rtspPort = 256,
    comPort = 512, panelPos = 1024, db = 2048, dbUser = 4096,
    traceSource = 8192, traceLevel = 16384, gstTraceLevel = 32768
};

using Switches = int;

struct OptionDescr
{
    Option option;
    const char * name;
    Switches switches;
};

struct SwitchDescr
{
    Switch switch_;
    const char * name;
    bool hasArg;
};

OptionDescr options[] = {
    {Option::start,         "start",            Switch::capturer | Switch::fps |
                                                Switch::scale | Switch::preset |
                                                Switch::bitrate | Switch::crf |
                                                Switch::keyint | Switch::intraRefresh |
                                                Switch::rtspPort | Switch::comPort |
                                                Switch::panelPos | Switch::db |
                                                Switch::dbUser | Switch::traceSource |
                                                Switch::traceLevel | Switch::gstTraceLevel },
    {Option::remove,        "remove",           Switch::traceSource | Switch::traceLevel},
    {Option::logfile,       "logfile",          0},
    {Option::console,       "console",          Switch::capturer | Switch::fps |
                                                Switch::scale | Switch::preset |
                                                Switch::bitrate | Switch::crf |
                                                Switch::keyint | Switch::intraRefresh |
                                                Switch::rtspPort | Switch::comPort |
                                                Switch::panelPos | Switch::db |
                                                Switch::dbUser | Switch::traceSource |
                                                Switch::traceLevel | Switch::gstTraceLevel },
    {Option::help,          "help",             0},
    {Option(0),             nullptr,            0}
};

SwitchDescr switches[] = {
    {Switch::capturer,      "--capturer",       true},
    {Switch::fps,           "--fps",            true},
    {Switch::scale,         "--scale",          true},
    {Switch::preset,        "--preset",         true},
    {Switch::bitrate,       "--bitrate",        true},
    {Switch::crf,           "--crf",            true},
    {Switch::keyint,        "--keyint",         true},
    {Switch::intraRefresh,  "--intra-refresh",  false},
    {Switch::rtspPort,      "--rtsp-port",      true},
    {Switch::comPort,       "--com-port",       true},
    {Switch::panelPos,      "--panel-pos",      true},
    {Switch::db,            "--db",             true},
    {Switch::dbUser,        "--db-user",        true},
    {Switch::traceSource,   "--trace-source",   false},
    {Switch::traceLevel,    "--trace-level",    true},
    {Switch::gstTraceLevel, "--gst-trace-level",true},
    {Switch(0),             nullptr,            false}
};

} // namespace

    //-- class Params --//

using Impl = Params::Impl;

Impl * Params::pImpl()
{
    static Impl impl;
    return &impl;
}

void Impl::init(int argc, char * argv[])
{
    option          = options[0].option;
    capturerType    = CapturerType::gdi;
    fps             = 5;
    scale           = {0, 0, 0, 0};
    preset          = "veryfast";
    bitrate         = 0; // no VBV bitrate control
    crf             = 35;
    crfMax          = 40;
    keyint          = 5;
    intraRefresh    = false;
    rtspPort        = 8554;
    comPort         = 0;
    panelPos        = {12, 12};
    dbHost          = "localhost";
    dbPort          = 3306;
    dbUser          = "";
    dbPass          = "";
    traceSource     = false;
    traceLevel      = 1;
    gstTraceLevel   = 1;

    int optionIdx = -1;
    for(int i = 1; i < argc; ++i) {
        int switchIdx = -1;
        if(argv[i][0] != '-') { // option
            int j;
            for(j = 0; options[j].name; ++j) {
                if(!strcmp(argv[i], options[j].name)) {
                    if(optionIdx >= 0 && optionIdx != j)
                        throw Err() << "Please specify a single option before any switch";
                    optionIdx = j;
                    option = options[optionIdx].option;
                    break;
                }
            }
            if(!options[j].name)
                throwUsage();
        } else { // switch
            int j;
            for(j = 0; switches[j].name; ++j)
                if(!strcmp(argv[i], switches[j].name)) {
                    if(optionIdx < 0)
                        optionIdx = 0;
                    if(!(options[optionIdx].switches & switches[j].switch_))
                        throw Err() << "Switch " << switches[j].name
                                    << " isn't applicable to option "
                                    << options[optionIdx].name;
                    switchIdx = j;
                    break;
                }
            if(!switches[j].name)
                throwUsage();
        }
        if(switchIdx >= 0) {
            if(switches[switchIdx].hasArg &&
                    (++i >= argc || argv[i][0] == '-'))
                throw Err() << "No value for switch "
                            << switches[switchIdx].name;
            parseSwitch(switchIdx, argv[i]);
        }
    }

    Msg::setLevel(traceLevel);
    Msg::setFilelines(traceSource);
}

void Impl::parseSwitch(int switchIdx, char * pSwitchArg)
{
    switch(switches[switchIdx].switch_) {
        case Switch::capturer:
        {
            if(!strcmpi(pSwitchArg, "GDI"))
                capturerType = CapturerType::gdi;
            else if(!strcmpi(pSwitchArg, "DX"))
                capturerType = CapturerType::dx;
            else if(!strcmpi(pSwitchArg, "null"))
                capturerType = CapturerType::null;
            else
                throw Err() << "Unknown capturer specified";
            break;
        }
        case Switch::fps:
        {
            Fps value;
            char * p = pSwitchArg;
            while(*p && *p != '/') ++p;
            if(*p) {
                *p++ = 0;
                value.den = atoi(p);
            } else {
                value.den = 1;
            }
            value.num = atoi(pSwitchArg);
            if(!value.isValid())
                throw Err() << "Invalid fps specified";
            fps = value;
            break;
        }
        case Switch::scale:
        {
            FrameScale value;
            char * p = pSwitchArg;
            while(*p && *p != '/') ++p;
            if(*p) {
                *p++ = 0;
                value.num = atoi(pSwitchArg);
                value.den = atoi(p);
                if(!value.num || !value.den)
                    throw Err() << "Invalid scale in format N/D specified";
                scale = value;
                break;
            }
            p = pSwitchArg;
            while(*p && *p != 'x') ++p;
            if(*p) {
                *p++ = 0;
                value.width = atoi(pSwitchArg);
                value.height = atoi(p);
                if(!value.width || !value.height)
                    throw Err() << "Invalid scale in format WxH specified";
                scale = value;
                break;
            }
            throw Err() << "Invalid scale specified";
            break;
        }
        case Switch::preset:
        {
            int i;
            for(i = 0; x264_preset_names[i]; ++i)
                if(!strcmp(pSwitchArg, x264_preset_names[i])) {
                    preset = pSwitchArg;
                    break;
                }
            if(!x264_preset_names[i])
                throw Err() << "Unknown preset specified";
            break;
        }
        case Switch::bitrate:
        {
            int value = atoi(pSwitchArg);
            if(value < 100 || value > 10000)
                throw Err() << "Invalid bitrate specified";
            bitrate = value;
            break;
        }
        case Switch::crf:
        {
            unsigned crfValue, crfMaxValue;
            char * p = pSwitchArg;
            while(*p && *p != '-') ++p;
            if(*p) {
                *p++ = 0;
                crfMaxValue = atoi(p);
            } else {
                crfMaxValue = crfMax;
            }
            crfValue = atoi(pSwitchArg);
            if(crfValue > 51 || crfMaxValue > 51)
                throw Err() << "Invalid crf specified";
            if(crfMaxValue < crfValue && *p)
                throw Err() << "Invalid crf interval specified";
            crfMax = std::max(crfValue, crfMaxValue);
            crf = crfValue;
            break;
        }
        case Switch::keyint:
        {
            int value = atoi(pSwitchArg);
            if(value < 1 || value > 250)
                throw Err() << "Invalid keyint parameter specified";
            keyint = value;
            break;
        }
        case Switch::intraRefresh:
        {
            intraRefresh = true;
            break;
        }
        case Switch::rtspPort:
        {
            int value = atoi(pSwitchArg);
            if(value < 1024 || value > 65535)
                throw Err() << "Invalid RTSP port specified";
            rtspPort = value;
            break;
        }
        case Switch::comPort:
        {
            int value = atoi(pSwitchArg);
            if(value < 1 || value > 8)
                throw Err() << "Invalid scales COM port specified";
            comPort = value;
            break;
        }
        case Switch::panelPos:
        {
            const char * valid = "01231456789";
            int last = strlen(pSwitchArg) - 1;
            if((pSwitchArg[0] == '(') != (pSwitchArg[last] == ')'))
                throw Err() << "Invalid parenthesis for info panel position specified";
            if(pSwitchArg[last] == ')')
                pSwitchArg[last] = 0;
            if(pSwitchArg[0] == '(')
                ++pSwitchArg;
            char * p = pSwitchArg;
            while(*p && *p != ',') ++p;
            if(!*p)
                throw Err() << "No comma provided in specified info panel position";
            *p++ = 0;
            char * p1 = pSwitchArg;
            if(*p1 == '-')
                ++p1;
            char * p2 = p;
            if(*p2 == '-')
                ++p2;
            if(p1[strspn(p1, valid)] || p2[strspn(p2, valid)])
                throw Err() << "Invalid number format for info panel position specified";
            panelPos.x = atoi(pSwitchArg);
            panelPos.y = atoi(p);
            break;
        }
        case Switch::db:
        {
            char * p = pSwitchArg;
            while(*p && *p != ':') ++p;
            if(p == pSwitchArg)
                throw Err() << "Empty DB host specified";
            if(*p) {
                *p++ = 0;
                int port = atoi(p);
                if(!port)
                    throw Err() << "Invalid DB port specified";
                dbPort = port;
            }
            dbHost = pSwitchArg;
            break;
        }
        case Switch::dbUser:
        {
            char * p = pSwitchArg;
            while(*p && *p != '/') ++p;
            if(p == pSwitchArg)
                throw Err() << "Empty DB user specified";
            if(*p) {
                *p++ = 0;
                dbPass = p;
            } else {
                dbPass = pSwitchArg;
            }
            dbUser = pSwitchArg;
            break;
        }
        case Switch::traceSource:
        {
            traceSource = true;
            break;
        }
        case Switch::traceLevel:
        {
            int value = atoi(pSwitchArg);
            if(value < 0 || value > 3)
                throw Err() << "Invalid trace level specified";
            traceLevel = value;
            break;
        }
        case Switch::gstTraceLevel:
        {
            int value = atoi(pSwitchArg);
            if(value < 0 || value > 9)
                throw Err() << "Invalid GStreamer trace level specified";
            gstTraceLevel = value;
            break;
        }
    }
}

void Impl::showUsage() const
{
    Msg() << "Usage:\n"
             "      wdvc.exe            Start process and place it to autorun\n"
             "      wdvc.exe remove     Stop process and remove it from autorun\n"
             "      wdvc.exe logfile    Show process log file name with path\n"
             "      wdvc.exe console    Start server in console\n"
             "      wdvc.exe help       Show usage\n"
             "Switches:\n"
             "      --capturer <GDI|DX|null>\n"
             "      --fps <numerator>[/<denominator>]\n"
             "      --scale <numerator>/<denominator>|<width>x<height>\n"
             "      --preset <ultrafast|superfast|veryfast|faster|fast|\n"
             "                medium|slow|slower|veryslow|placebo>\n"
             "      --bitrate <bitrate in kbps>\n"
             "      --crf <crf[-crfmax]>\n"
             "      --keyint <keyint>\n"
             "      --intra-refresh\n"
             "      --rtsp-port <RTSP port>\n"
             "      --com-port <scales' COM port>\n"
             "      --panel-pos (<x>,<y>)\n"
             "      --db <dbname[@dbhost[:dbport]]>\n"
             "      --db-user <dbuser[/dbpass]>\n"
             "      --trace-source\n"
             "      --trace-level <trace level>\n"
             "      --gst-trace-level <gst trace level>";
}

void Impl::throwUsage() const
{
    showUsage();
    throw Err() << "Invalid option or switch specified";
}
