###############################################################
#                                                             #
#  MAKEFILE for Win32 GPIB toolkit                            #
#                                                             #
#  Execute with Microsoft (or compatible) NMAKE               #
#                                                             #
###############################################################

#
# Options used with MSVC 13 to target Win9x
#

cc = cl
cdebug = -Zi -Od -DDEBUG -MTd
crelease = -Ox -MT

cflags = -c -DCRTAPI1=_cdecl -DCRTAPI2=_cdecl -nologo \
         -D_X86_=1 -DWIN32 -D_WIN32 -W3 -D_WIN95 \
         -D_WIN32_WINDOWS=0x0400 -D_WIN32_IE=0x400 -DWINVER=0x400 \
         -D_MT -D_CRT_SECURE_NO_DEPRECATE /Fd$(OBJDIR)\

link = link
ldebug = /DEBUG /DEBUGTYPE:cv
guilflags = /INCREMENTAL:NO /NOLOGO -subsystem:windows,6.0 /LIBPATH:lib /LIBPATH:gpiblib\lib
conlflags = /INCREMENTAL:NO /NOLOGO -subsystem:console,6.0 /LIBPATH:lib /LIBPATH:gpiblib\lib
dlllflags = /INCREMENTAL:NO /NOLOGO -subsystem:windows,6.0 -entry:_DllMainCRTStartup@12 -dll /LIBPATH:lib /LIBPATH:gpiblib\lib
guilibsmt = kernel32.lib ws2_32.lib mswsock.lib advapi32.lib user32.lib gdi32.lib comdlg32.lib
conlibsmt = kernel32.lib ws2_32.lib mswsock.lib advapi32.lib user32.lib

#
# Targets
#

OBJDIR = obj
BINDIR = bin
LIBDIR = lib

all: dirs \
     $(BINDIR)\gpiblib.dll     \
     $(BINDIR)\7470.exe        \
     $(BINDIR)\7470.ini        \
     $(BINDIR)\parse.exe       \
     $(BINDIR)\printhpg.exe    \
     $(BINDIR)\query.exe       \
     $(BINDIR)\talk.exe        \
     $(BINDIR)\listen.exe      \
     $(BINDIR)\ntpquery.exe    \
     $(BINDIR)\tcplist.exe     \
     $(BINDIR)\readpkr.exe     \
     $(BINDIR)\readinst.exe    \
     $(BINDIR)\binquery.exe    \
     $(BINDIR)\atten_3488a.exe \
     $(BINDIR)\44471A.exe      \
     $(BINDIR)\5071a.exe       \
     $(BINDIR)\t962.exe        \
     $(BINDIR)\5370.exe        \
     $(BINDIR)\5345a.exe       \
     $(BINDIR)\8672a.exe       \
     $(BINDIR)\11848a.exe      \
     $(BINDIR)\dts2070.exe     \
     $(BINDIR)\dso6000.exe     \
     $(BINDIR)\psaplot.exe     \
     $(BINDIR)\hp3458.exe      \
     $(BINDIR)\sgentest.exe    \
     $(LIBDIR)\specan.lib      \
     $(BINDIR)\gps.exe         \
     $(LIBDIR)\gnss.lib        \
     $(BINDIR)\cal_tek490.exe  \
     $(BINDIR)\txt2pnp.exe     \
     $(BINDIR)\satrace.exe     \
     $(BINDIR)\pn.exe          \
     $(BINDIR)\pn.ini          \
     $(BINDIR)\ssm.exe         \
     $(BINDIR)\ssmdump.exe     \
     $(BINDIR)\ssmchan.exe     \
     $(BINDIR)\tcheck.exe      \
     $(BINDIR)\prologix.exe    \
     $(BINDIR)\vna.exe         \
     $(BINDIR)\dataq.exe       \
     $(BINDIR)\visadmm.exe     \
     $(BINDIR)\mx20.exe        \
     $(BINDIR)\txcom.exe       \
     $(BINDIR)\echoserver.exe  \
     $(BINDIR)\echoclient.exe  \
     $(BINDIR)\mtie.exe

dirs:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(BINDIR) mkdir $(BINDIR)
	@if not exist $(LIBDIR) mkdir $(LIBDIR)

#
# GPIB Library
#

$(OBJDIR)\gpiblib.obj: gpiblib\gpiblib.cpp gpiblib\decl-32.h gpiblib\gpiblib.h gpiblib\comblock.h
	$(cc) $(cdebug) $(cflags) /Fo$@ gpiblib\gpiblib.cpp

$(BINDIR)\gpiblib.dll $(LIBDIR)\gpiblib.lib: $(OBJDIR)\gpiblib.obj gpiblib\gpib-32.obj
	$(link) $(ldebug) $(dlllflags) $(guilibsmt) $(OBJDIR)\gpiblib.obj gpiblib\gpib-32.obj winmm.lib kernel32.lib user32.lib shell32.lib /OUT:$(BINDIR)\gpiblib.dll /IMPLIB:$(LIBDIR)\gpiblib.lib

#
# Optional standalone console app for test development, not built by default
#

$(BINDIR)\gpiblib.exe: $(OBJDIR)\gpiblib.obj gpiblib\gpib-32.obj
	$(link) $(ldebug) $(conlflags) $(guilibsmt) $(OBJDIR)\gpiblib.obj gpiblib\gpib-32.obj winmm.lib kernel32.lib user32.lib shell32.lib /OUT:$@

#
# HP7470A emulator app
#

$(OBJDIR)\7470.res: 7470.ico 7470.rc 7470res.h
	rc /fo$@ 7470.rc

$(BINDIR)\7470.exe: $(OBJDIR)\7470.obj winvfx16.lib w32sal.lib $(LIBDIR)\gpiblib.lib $(LIBDIR)\specan.lib $(OBJDIR)\7470.res
	@echo Building 7470.exe
	$(link) $(ldebug) $(guilflags) -out:$@ $(OBJDIR)\7470.obj $(LIBDIR)\specan.lib w32sal.lib winvfx16.lib $(LIBDIR)\gpiblib.lib $(guilibsmt) winmm.lib shell32.lib $(OBJDIR)\7470.res

$(OBJDIR)\7470.obj: 7470.cpp w32sal.h winvfx.h gpiblib.h 7470lex.cpp 8566plt.h 49xplt.h specan.h 7470res.h appfile.cpp renderer.cpp
	$(cc) $(cdebug) $(cflags) $(appflags) /Fo$@ 7470.cpp

$(BINDIR)\7470.ini: default_7470.ini
	copy default_7470.ini $@

#
# Phase-noise measurement app
# (normally built in release mode for improved graph-processing speed)
#

$(OBJDIR)\pn.res: pn.ico pn.rc pnres.h
	rc /fo$@ pn.rc

$(BINDIR)\pn.exe: $(OBJDIR)\pn.obj winvfx16.lib w32sal.lib $(LIBDIR)\gpiblib.lib $(OBJDIR)\pn.res
	@echo Building pn.exe
	$(link) $(ldebug) $(guilflags) -out:$@ $(OBJDIR)\pn.obj w32sal.lib winvfx16.lib $(LIBDIR)\gpiblib.lib $(guilibsmt) /NODEFAULTLIB:libcmtd.lib winmm.lib shell32.lib $(OBJDIR)\pn.res

$(OBJDIR)\pn.obj: pn.cpp w32sal.h winvfx.h gpiblib.h pnres.h
	$(cc) $(crelease) $(cflags) $(appflags) /Fo$@ pn.cpp

$(BINDIR)\pn.ini: default_pn.ini
	copy default_pn.ini $@

#
# TSC-51xxA text dump to .PNP conversion
#

$(BINDIR)\txt2pnp.exe: txt2pnp.cpp
	@echo Building txt2pnp.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\txt2pnp.obj txt2pnp.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\txt2pnp.obj $(conlibsmt)

#
# Calibration routines from Tektronix 492/492P service volume 1
# (070-3783-01)
#

$(BINDIR)\cal_tek490.exe: cal_tek490.cpp $(LIBDIR)\gpiblib.lib
	@echo Building cal_tek490.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\cal_tek490.obj cal_tek490.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\cal_tek490.obj $(LIBDIR)\gpiblib.lib $(conlibsmt)

#
# HPGL parser for testing
#

$(BINDIR)\parse.exe: parse.cpp 7470lex.cpp
	@echo Building parse.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\parse.obj parse.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\parse.obj $(conlibsmt) winmm.lib

#
# Send HP-GL/2 file to PRN device
#

$(BINDIR)\printhpg.exe: printhpg.cpp 7470lex.cpp
	@echo Building printhpg.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\printhpg.obj printhpg.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\printhpg.obj $(conlibsmt) winmm.lib

#
# Submit query to instrument
#

$(BINDIR)\query.exe: query.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building query.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\query.obj query.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\query.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Submit query to instrument, saving binary result to file
#

$(BINDIR)\binquery.exe: binquery.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building binquery.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\binquery.obj binquery.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\binquery.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Issue command to instrument
#

$(BINDIR)\talk.exe: talk.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building talk.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\talk.obj talk.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\talk.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Monitor bus in listen-only mode
#

$(BINDIR)\listen.exe: listen.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building listen.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\listen.obj listen.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\listen.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Query time from NTP server
#

$(BINDIR)\ntpquery.exe: ntpquery.cpp timeutil.cpp ipconn.cpp
	@echo Building ntpquery.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\ntpquery.obj ntpquery.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\ntpquery.obj $(conlibsmt) winmm.lib wininet.lib

#
# Monitor IP connection in listen-only mode
#

$(BINDIR)\tcplist.exe: tcplist.cpp ipconn.cpp timeutil.cpp
	@echo Building tcplist.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\tcplist.obj tcplist.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\tcplist.obj $(conlibsmt) winmm.lib ws2_32.lib

#
# Read Inficon PKR251 pressure sensor using HP 34410A over TCP/IP
#

$(BINDIR)\readpkr.exe: readpkr.cpp ipconn.cpp timeutil.cpp
	@echo Building readpkr.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\readpkr.obj readpkr.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\readpkr.obj $(conlibsmt) winmm.lib ws2_32.lib

#
# Read HP 34461A or similar instrument that responds to Telnet-style READ? queries over TCP/IP
# (usually port 5025 is appropriate)
#

$(BINDIR)\readinst.exe: readinst.cpp ipconn.cpp timeutil.cpp appfile.cpp stdtpl.h
	@echo Building readinst.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\readinst.obj readinst.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\readinst.obj $(conlibsmt) winmm.lib ws2_32.lib

#
# Test signal generator at random frequencies and amplitudes
#

$(BINDIR)\sgentest.exe: sgentest.cpp gpiblib.h $(LIBDIR)\gpiblib.lib $(LIBDIR)\specan.lib
	@echo Building sgentest.exe
	cl $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T /Fo$(OBJDIR)\sgentest.obj sgentest.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\sgentest.obj $(LIBDIR)\gpiblib.lib $(LIBDIR)\specan.lib $(conlibsmt) winmm.lib

#
# Program 33004A/33005A attenuators connected to HP 3488A / 44470A switch box
#

$(BINDIR)\atten_3488a.exe: atten_3488a.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building atten_3488a.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\atten_3488a.obj atten_3488a.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\atten_3488a.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Control relays on 44471A card in HP 3448A chassis
#

$(BINDIR)\44471A.exe: 44471A.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building 44471A.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\44471A.obj 44471A.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\44471A.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Set HP 5071A clock to NTP server time
#

$(BINDIR)\5071a.exe: 5071a.cpp timeutil.cpp comport.cpp ipconn.cpp
	@echo Building 5071a.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\5071a.obj 5071a.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\5071a.obj $(conlibsmt) winmm.lib user32.lib

#
# MX20 comms test
#

$(BINDIR)\mx20.exe: MX20.cpp timeutil.cpp comport.cpp
	@echo Building MX20.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\MX20.obj MX20.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\MX20.obj $(conlibsmt) winmm.lib user32.lib

#
# Send string to serial port
#

$(BINDIR)\txcom.exe: txcom.cpp timeutil.cpp comport.cpp
	@echo Building txcom.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\txcom.obj txcom.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\txcom.obj $(conlibsmt) winmm.lib user32.lib

#
# Read data from T962 oven with ESTechnical controller
#

$(BINDIR)\t962.exe: t962.cpp timeutil.cpp comport.cpp
	@echo Building t962.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\t962.obj t962.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\t962.obj $(conlibsmt) winmm.lib user32.lib

#
# Request current frequency from HP 5370A/B
#

$(BINDIR)\5370.exe: 5370.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building 5370.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\5370.obj 5370.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\5370.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Request current frequency from HP 5345A
#

$(BINDIR)\5345a.exe: 5345a.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building 5345a.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\5345a.obj 5345a.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\5345a.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Set freq/amplitude on HP 8672A
#

$(BINDIR)\8672a.exe: 8672a.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building 8672a.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\8672a.obj 8672a.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\8672a.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Control various features in HP 11848a
#

$(BINDIR)\11848a.exe: 11848a.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building 11848a.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\11848a.obj 11848a.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\11848a.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Trigger and take readings from Wavecrest DTS 207x digital time analyzer
#

$(BINDIR)\dts2070.exe: dts2070.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building dts2070.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\dts2070.obj dts2070.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\dts2070.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Acquire .GIF screen dump from Agilent E4406A vector signal analyzer
#

$(BINDIR)\psaplot.exe: psaplot.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building psaplot.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\psaplot.obj psaplot.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\psaplot.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Trigger and take readings from Agilent DSO/MSO model
# (tested with MSO6054A)
#

$(BINDIR)\dso6000.exe: dso6000.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building dso6000.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\dso6000.obj dso6000.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\dso6000.obj $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Back up calibration and data NVRAMs from HP 3458A multimeter
#

$(BINDIR)\hp3458.exe: hp3458.cpp gpiblib.h $(LIBDIR)\gpiblib.lib
	@echo Building hp3458.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\hp3458.obj hp3458.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\hp3458.obj $(LIBDIR)\gpiblib.lib $(conlibsmt)

#
# DLL to fetch raw binary traces from spectrum analyzers
#

$(OBJDIR)\specan.obj: specan.cpp spline.cpp specan.h gpiblib.h
		@echo Building specan.obj
		$(cc) $(cdebug) $(cflags) /Fo$@ specan.cpp

$(BINDIR)\specan.dll $(LIBDIR)\specan.lib: $(OBJDIR)\specan.obj $(LIBDIR)\gpiblib.lib
		@echo Building specan.lib
		$(link) $(ldebug) $(dlllflags) $(guilibsmt) $(OBJDIR)\specan.obj sh_api.lib $(LIBDIR)\gpiblib.lib kernel32.lib user32.lib winmm.lib /out:$(BINDIR)\specan.dll /IMPLIB:$(LIBDIR)\specan.lib

#
# DLL to fetch fix information from various GNSS receivers (NMEA GPS, etc.)
#

$(OBJDIR)\gnss.obj: gnss.cpp gnss.h gpiblib\comblock.h
		@echo Building gnss.obj
		$(cc) $(cdebug) $(cflags) /Fo$@ gnss.cpp

$(BINDIR)\gnss.dll $(LIBDIR)\gnss.lib: $(OBJDIR)\gnss.obj $(LIBDIR)\gpiblib.lib
		@echo Building gnss.lib
		$(link) $(ldebug) $(dlllflags) $(guilibsmt) $(OBJDIR)\gnss.obj kernel32.lib user32.lib winmm.lib /out:$(BINDIR)\gnss.dll /IMPLIB:$(LIBDIR)\gnss.lib

#
# Display GNSS fixes from GPS or other GNSS sources
#

$(BINDIR)\gps.exe: gps.cpp gnss.h $(LIBDIR)\gnss.lib
	@echo Building gps.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\gps.obj gps.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\gps.obj $(LIBDIR)\gnss.lib $(conlibsmt) winmm.lib

#
# Request .CSV traces from spectrum analyzers
#

$(BINDIR)\satrace.exe: satrace.cpp specan.h $(LIBDIR)\specan.lib $(LIBDIR)\gpiblib.lib
	@echo Building satrace.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\satrace.obj satrace.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\satrace.obj $(LIBDIR)\specan.lib $(LIBDIR)\gpiblib.lib $(conlibsmt) winmm.lib

#
# Fetch and scale data from a DataQ DI-154RS or DI-194RS serial DAQ unit
#

$(BINDIR)\dataq.exe: dataq.cpp timeutil.cpp comport.cpp di154.cpp
	@echo Building dataq.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\dataq.obj dataq.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\dataq.obj $(conlibsmt) winmm.lib user32.lib

#
# Stream readings from an HP 34461A or similar VISA-enabled DMM
#

$(BINDIR)\visadmm.exe: visadmm.cpp timeutil.cpp
	@echo Building visadmm.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\visadmm.obj visadmm.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\visadmm.obj agvisa32.lib $(conlibsmt) winmm.lib user32.lib

#
# Generic test app
#

$(BINDIR)\t.exe: t.cpp timeutil.cpp
	@echo Building t.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\t.obj t.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\t.obj $(conlibsmt) winmm.lib user32.lib

#
# Surveillance monitor program and display components
#

$(OBJDIR)\ssm.res: ssm.ico ssm.rc ssmres.h
	rc /fo$@ ssm.rc

$(BINDIR)\ssm.exe: $(OBJDIR)\ssm.obj $(OBJDIR)\xy.obj $(OBJDIR)\waterfall.obj $(OBJDIR)\recorder.obj winvfx16.lib w32sal.lib $(LIBDIR)\specan.lib $(LIBDIR)\gnss.lib $(OBJDIR)\ssm.res
	@echo Building ssm.exe
	$(link) $(ldebug) $(guilflags) -out:$@ $(OBJDIR)\ssm.obj $(OBJDIR)\xy.obj $(OBJDIR)\waterfall.obj $(OBJDIR)\recorder.obj w32sal.lib winvfx16.lib $(LIBDIR)\specan.lib $(LIBDIR)\gnss.lib $(guilibsmt) winmm.lib shell32.lib $(OBJDIR)\ssm.res

$(BINDIR)\ssmdump.exe: $(OBJDIR)\ssmdump.obj $(OBJDIR)\recorder.obj $(LIBDIR)\gnss.lib
	@echo Building ssmdump.exe
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\ssmdump.obj $(OBJDIR)\recorder.obj $(LIBDIR)\gnss.lib $(conlibsmt) winmm.lib

$(BINDIR)\ssmchan.exe: $(OBJDIR)\ssmchan.obj $(OBJDIR)\recorder.obj $(LIBDIR)\gnss.lib
	@echo Building ssmchan.exe
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\ssmchan.obj $(OBJDIR)\recorder.obj $(LIBDIR)\gnss.lib $(conlibsmt) winmm.lib

$(OBJDIR)\xy.obj: xy.h xy.cpp winvfx.h w32sal.h
	$(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T /Fo$@ xy.cpp

$(OBJDIR)\waterfall.obj: waterfall.h waterfall.cpp winvfx.h w32sal.h
	$(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T /Fo$@ waterfall.cpp

$(OBJDIR)\recorder.obj: recorder.h recorder.cpp winvfx.h w32sal.h
	$(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T /Fo$@ recorder.cpp

$(OBJDIR)\ssm.obj: ssm.cpp w32sal.h winvfx.h specan.h gnss.h xy.h waterfall.h recorder.h ssmres.h
	$(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T /Fo$@ ssm.cpp

$(OBJDIR)\ssmdump.obj: ssmdump.cpp recorder.h
	$(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T /Fo$@ ssmdump.cpp

$(OBJDIR)\ssmchan.obj: ssmchan.cpp recorder.h
	$(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T /Fo$@ ssmchan.cpp

#
# VNA performance test
#

$(OBJDIR)\tcheck.res: tcheck.ico tcheck.rc tchkres.h
	rc /fo$@ tcheck.rc

$(BINDIR)\tcheck.exe: $(OBJDIR)\tcheck.obj $(OBJDIR)\xy.obj winvfx16.lib w32sal.lib $(OBJDIR)\tcheck.res
	@echo Building tcheck.exe
	$(link) $(ldebug) $(guilflags) -out:$@ $(OBJDIR)\tcheck.obj $(OBJDIR)\xy.obj w32sal.lib winvfx16.lib $(guilibsmt) winmm.lib shell32.lib $(OBJDIR)\tcheck.res

$(OBJDIR)\tcheck.obj: tcheck.cpp spline.cpp sparams.cpp w32sal.h winvfx.h xy.h tchkres.h
	$(cc) $(cdebug) $(cflags) $(appflags) /Fo$@ tcheck.cpp

#
# Prologix/CONNECT.INI configurator
#

$(OBJDIR)\prologix.res: gpib.ico prologix.rc prores.h
	rc /fo$@ prologix.rc

$(BINDIR)\prologix.exe: $(OBJDIR)\prologix.obj $(OBJDIR)\prologix.res
	@echo Building prologix.exe
	$(link) $(ldebug) $(guilflags) -out:$@ $(OBJDIR)\prologix.obj gpiblib/gpib-32.obj $(guilibsmt) winmm.lib shell32.lib comctl32.lib ws2_32.lib setupapi.lib $(OBJDIR)\prologix.res

$(OBJDIR)\prologix.obj: prologix.cpp typedefs.h prores.h gpiblib\comblock.h
	$(cc) $(cdebug) $(cflags) $(appflags) /Fo$@ prologix.cpp

#
# HP 8510/8753-series VNA state/calibration maintenance utility
#

$(OBJDIR)\vna.res: vna.ico vna.rc vnares.h
	rc /fo$@ vna.rc

$(BINDIR)\vna.exe: $(OBJDIR)\vna.obj $(LIBDIR)\gpiblib.lib $(OBJDIR)\vna.res
	@echo Building vna.exe
	$(link) $(ldebug) $(guilflags) -out:$@ $(OBJDIR)\vna.obj $(LIBDIR)\gpiblib.lib $(guilibsmt) winmm.lib comctl32.lib $(OBJDIR)\vna.res

$(OBJDIR)\vna.obj: vna.cpp sparams.cpp spline.cpp typedefs.h gpiblib.h vnares.h
	$(cc) $(cdebug) $(cflags) $(appflags) /Fo$@ vna.cpp

#
# TCP/IP loopback benchmark/diagnostic
#

$(BINDIR)\echoserver.exe: echoserver.cpp
	@echo Building echoserver.exe
	cl -EHsc -D_CRT_SECURE_NO_DEPRECATE /Fe$@ echoserver.cpp user32.lib ws2_32.lib

$(BINDIR)\echoclient.exe: echoclient.cpp gpiblib/comblock.h
	@echo Building echoclient.exe
	cl $(cdebug) $(cflags) $(appflags) /Fo$(OBJDIR)\echoclient.obj echoclient.cpp
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJDIR)\echoclient.obj $(conlibsmt) ws2_32.lib winmm.lib user32.lib

#
# Other tests
#

$(BINDIR)\mtie.exe: mtie.cpp
	@echo Building mtie.exe
	cl -EHsc /Fe$@ mtie.cpp user32.lib

#
# Client setup program (requires Inno Setup installation, see
# www.jrsoftware.org)
#

output\setup.exe: gpib.iss
	copy default_pn.ini pn.ini
	copy default_connect.ini connect.ini
	copy default_7470.ini 7470.ini
	copy default_colors.bin colors.bin
	c:\progra~2\innose~1\iscc /Q gpib.iss

clean:
	if exist $(OBJDIR) rmdir /S /Q $(OBJDIR)
	if exist $(BINDIR) rmdir /S /Q $(BINDIR)
	if exist $(LIBDIR) rmdir /S /Q $(LIBDIR)
	del *.exe *.obj *.res *.pdb specan.lib gnss.lib 7470.ini pn.ini
