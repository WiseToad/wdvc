	NAME

Short name: wdvc
Full name: Windows Desktop Video Capturer

	DESCRIPTION

General purpose is to capture Windows Desktop in background, then encode
captured stream in raw h.264 format and then send it over network using
RTSP protocol

	RELEASE

0.23 October 2017

	TARGET PLATFORMS

Windows XP, 7, 10 x32/x64

	BUILD TOOLS

QtCreator 4.0.3 Community, MinGW 4.9.1 from Qt 5.6.3 package
https://www.qt.io/download-open-source

MSYS (used to work with MinGW above for support libraries building)
http://www.mingw.org

pkg-config 0.28
https://www.freedesktop.org/wiki/Software/pkg-config

fasm 1.71.63
https://flatassembler.net

	BUILD LIBS AND SDK

ffmpeg 3.3.4, built from source
https://ffmpeg.org

x264 0.152.2851 ba24899, built from source
http://www.videolan.org/developers/x264.html

GStreamer 1.6.4 32bit
https://gstreamer.freedesktop.org

MySQL Connector C 6.1.10 32bit
https://www.mysql.com/products/connector

	DOCUMENT CONVENTIONS

Following placeholders will be used further:

<MSYS_PATH>	    Path where MSYS has been installed
<GSTREAMER_PATH>    Path where GStreamer SDK has been installed
<WDVC_PATH>	    Path where target EXE will be built

	BUILD NOTES

Environment variable PKG_CONFIG_PATH must include paths to .pc files which came
with x264, ffmpeg and GStreamer. If libs that has been built from source reside
under /usr/local (i.e. <MSYS_PATH>\msys\1.0\local), variable can be as follows:
<MSYS_PATH>\msys\1.0\local\lib\pkgconfig;<GSTREAMER_PATH>\1.0\x86\lib\pkgconfig

Also you probably need to change variables at the very top of the .pro file

	TARGET DEPENDENCIES

Copy files listed below from source paths to target paths to bring target EXE to
(properly) working state:

Source path: <GSTREAMER_PATH>\1.0\x86\bin 
Target path: <WDVC_PATH>
	libffi-6.dll
	libgio-2.0-0.dll
	libglib-2.0-0.dll
	libgmodule-2.0-0.dll
	libgobject-2.0-0.dll
	libgstapp-1.0-0.dll
	libgstaudio-1.0-0.dll
	libgstbase-1.0-0.dll
	libgstnet-1.0-0.dll
	libgstpbutils-1.0-0.dll
	libgstreamer-1.0-0.dll
	libgstrtp-1.0-0.dll
	libgstrtsp-1.0-0.dll
	libgstrtspserver-1.0-0.dll
	libgstsdp-1.0-0.dll
	libgsttag-1.0-0.dll
	libgstvideo-1.0-0.dll
	libintl-8.dll
	libmysql.dll
	liborc-0.4-0.dll
	libwinpthread-1.dll
	libx264-148.dll
	libz.dll

Source path: <GSTREAMER_PATH>\1.0\x86\lib\gstreamer-1.0
Target path: <WDVC_PATH>\plugins
	libgstapp.dll
	libgstcoreelements.dll
	libgstrtp.dll
	libgstrtpmanager.dll
	libgstudp.dll
	libgstvideoconvert.dll
	libgstvideoscale.dll
	libgstx264.dll

Also, release must contain latest VS 2015 redist installer on which MySQL library depends
