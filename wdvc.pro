# Adjust following variables according to your setup
MINGW_PATH = C:/Programs/Qt/Qt-5.6.3/Tools/mingw492_32
MYSQL_PATH = C:/Programs/SDK/MySQL/mysql-connector-c-6.1.10-win32

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += c++14

QMAKE_CXXFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -Wno-comment -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unknown-pragmas

QMAKE_LFLAGS += -Wl,--enable-stdcall-fixup \
    -static-libgcc -static-libstdc++

INCLUDEPATH += \
    # Qt 5.6.3's GCC 4.9.2 doesn't need it:
    # $${MINGW_PATH}/include/mingw-std-threads \
    $${MYSQL_PATH}/include

LIBS += \ # System library paths prioretization
    -L$${MINGW_PATH}/lib \
    -L$${MINGW_PATH}/lib/gcc/i686-w64-mingw32/4.9.2

LIBS += \
    -lgdi32 -lws2_32 -lshlwapi -lole32 \
    -ld3d9 -ldsetup

LIBS += \
    -L$$MYSQL_PATH/lib \
    -lmysql

CONFIG += link_pkgconfig
PKGCONFIG += libswscale libavutil x264 \
    gstreamer-1.0 gstreamer-rtsp-server-1.0 gstreamer-app-1.0

SOURCES += \
    main.cpp \
    Capturer.cpp \
    SysHandler.cpp \
    Daemon.cpp \
    Common.cpp \
    Params.cpp \
    Server.cpp \
    Frame.cpp \
    Scales.cpp \
    Msg.cpp \
    Encoder.cpp

HEADERS += \
    Capturer.h \
    SysHandler.h \
    ImageRes.h \
    Frame.h \
    Handle.h \
    Daemon.h \
    Buffer.h \
    Common.h \
    Params.h \
    Server.h \
    Scales.h \
    Msg.h \
    Stateful.h \
    Blend.h \
    Timing.h \
    Guard.h \
    Text.h \
    TypeTraits.h \
    Gdi.h \
    Win.h \
    GStreamer.h \
    DirectX.h \
    MySQL.h \
    Encoder.h \
    Sink.h \
    x264.h \
    ffmpeg.h \
    Ptr.h

DISTFILES += \
    Blend.asm

ASM_SOURCES = $$find(DISTFILES, .*\.asm)

fasm.name = fasm
fasm.input = ASM_SOURCES
fasm.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}.o
fasm.commands = fasm ${QMAKE_FILE_NAME} ${QMAKE_FILE_OUT}

QMAKE_EXTRA_COMPILERS += fasm
