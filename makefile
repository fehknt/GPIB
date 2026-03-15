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
         -D_MT -D_CRT_SECURE_NO_DEPRECATE

link = link
ldebug = /DEBUG /DEBUGTYPE:cv
guilflags = /INCREMENTAL:NO /NOLOGO -subsystem:windows,6.0
conlflags = /INCREMENTAL:NO /NOLOGO -subsystem:console,6.0
dlllflags = /INCREMENTAL:NO /NOLOGO -subsystem:windows,6.0 -entry:_DllMainCRTStartup@12 -dll
guilibsmt = kernel32.lib ws2_32.lib mswsock.lib advapi32.lib user32.lib gdi32.lib comdlg32.lib
conlibsmt = kernel32.lib ws2_32.lib mswsock.lib advapi32.lib user32.lib

#
# Targets
#

all: 7470.exe        \
     7470.ini        \
     parse.exe       \
     printhpg.exe    \
     query.exe       \
     talk.exe        \
     listen.exe      \
     ntpquery.exe    \
     tcplist.exe     \
     readpkr.exe     \
     readinst.exe    \
     binquery.exe    \
     atten_3488a.exe \
     44471A.exe      \
     5071a.exe       \
     t962.exe        \
     5370.exe        \
     5345a.exe       \
     8672a.exe       \
     11848a.exe      \
     dts2070.exe     \
     dso6000.exe     \
     psaplot.exe     \
     hp3458.exe      \
     sgentest.exe    \
     specan.lib      \
     gps.exe         \
     gnss.lib        \
     cal_tek490.exe  \
     txt2pnp.exe     \
     satrace.exe     \
     pn.exe          \
     pn.ini          \
     ssm.exe         \
     ssmdump.exe     \
     ssmchan.exe     \
     tcheck.exe      \
     prologix.exe    \
     vna.exe         \
     dataq.exe       \
     visadmm.exe     \
     mx20.exe        \
     txcom.exe       \
     echoserver.exe  \
     echoclient.exe  \
     mtie.exe

#
# HP7470A emulator app
#

7470.res: 7470.ico 7470.rc 7470res.h
   rc 7470.rc

7470.exe: 7470.obj winvfx16.lib w32sal.lib gpiblib.lib specan.lib 7470.res
	@echo Building 7470.exe
	$(link) $(ldebug) $(guilflags) -out:7470.exe 7470.obj specan.lib w32sal.lib winvfx16.lib gpiblib.lib $(guilibsmt) winmm.lib shell32.lib 7470.res

7470.obj: 7470.cpp w32sal.h winvfx.h gpiblib.h 7470lex.cpp 8566plt.h 49xplt.h specan.h 7470res.h appfile.cpp renderer.cpp
    $(cc) $(cdebug) $(cflags) $(appflags) 7470.cpp

7470.ini: default_7470.ini
   copy default_7470.ini 7470.ini

#
# Phase-noise measurement app
# (normally built in release mode for improved graph-processing speed)
#

pn.res: pn.ico pn.rc pnres.h
   rc pn.rc

pn.exe: pn.obj winvfx16.lib w32sal.lib gpiblib.lib pn.res
	@echo Building pn.exe
	$(link) $(ldebug) $(guilflags) -out:pn.exe pn.obj w32sal.lib winvfx16.lib gpiblib.lib $(guilibsmt) /NODEFAULTLIB:libcmtd.lib winmm.lib shell32.lib pn.res

pn.obj: pn.cpp w32sal.h winvfx.h gpiblib.h pnres.h
    $(cc) $(crelease) $(cflags) $(appflags) pn.cpp

pn.ini: default_pn.ini
   copy default_pn.ini pn.ini

#
# TSC-51xxA text dump to .PNP conversion
#

txt2pnp.exe: txt2pnp.cpp
	@echo Building txt2pnp.exe
	cl $(cdebug) $(cflags) $(appflags) txt2pnp.cpp
	$(link) $(ldebug) $(conlflags) -out:txt2pnp.exe txt2pnp.obj $(conlibsmt)

#
# Calibration routines from Tektronix 492/492P service volume 1
# (070-3783-01)
#

cal_tek490.exe: cal_tek490.cpp gpiblib.lib
	@echo Building cal_tek490.exe
	cl $(cdebug) $(cflags) $(appflags) cal_tek490.cpp
	$(link) $(ldebug) $(conlflags) -out:cal_tek490.exe cal_tek490.obj gpiblib.lib $(conlibsmt)

#
# HPGL parser for testing
#

parse.exe: parse.cpp 7470lex.cpp
	@echo Building parse.exe
	cl $(cdebug) $(cflags) $(appflags) parse.cpp
	$(link) $(ldebug) $(conlflags) -out:parse.exe parse.obj $(conlibsmt) winmm.lib

#
# Send HP-GL/2 file to PRN device
#

printhpg.exe: printhpg.cpp 7470lex.cpp
	@echo Building printhpg.exe
	cl $(cdebug) $(cflags) $(appflags) printhpg.cpp
	$(link) $(ldebug) $(conlflags) -out:printhpg.exe printhpg.obj $(conlibsmt) winmm.lib

#
# Submit query to instrument
#

query.exe: query.cpp gpiblib.h gpiblib.lib
	@echo Building query.exe
	cl $(cdebug) $(cflags) $(appflags) query.cpp
	$(link) $(ldebug) $(conlflags) -out:query.exe query.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Submit query to instrument, saving binary result to file
#

binquery.exe: binquery.cpp gpiblib.h gpiblib.lib
	@echo Building binquery.exe
	cl $(cdebug) $(cflags) $(appflags) binquery.cpp
	$(link) $(ldebug) $(conlflags) -out:binquery.exe binquery.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Issue command to instrument
#

talk.exe: talk.cpp gpiblib.h gpiblib.lib
	@echo Building talk.exe
	cl $(cdebug) $(cflags) $(appflags) talk.cpp
	$(link) $(ldebug) $(conlflags) -out:talk.exe talk.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Monitor bus in listen-only mode
#

listen.exe: listen.cpp gpiblib.h gpiblib.lib
	@echo Building listen.exe
	cl $(cdebug) $(cflags) $(appflags) listen.cpp
	$(link) $(ldebug) $(conlflags) -out:listen.exe listen.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Query time from NTP server
#

ntpquery.exe: ntpquery.cpp timeutil.cpp ipconn.cpp
	@echo Building ntpquery.exe
	cl $(cdebug) $(cflags) $(appflags) ntpquery.cpp
	$(link) $(ldebug) $(conlflags) -out:ntpquery.exe ntpquery.obj $(conlibsmt) winmm.lib wininet.lib

#
# Monitor IP connection in listen-only mode
#

tcplist.exe: tcplist.cpp ipconn.cpp timeutil.cpp
	@echo Building tcplist.exe
	cl $(cdebug) $(cflags) $(appflags) tcplist.cpp
	$(link) $(ldebug) $(conlflags) -out:tcplist.exe tcplist.obj $(conlibsmt) winmm.lib ws2_32.lib

#
# Read Inficon PKR251 pressure sensor using HP 34410A over TCP/IP
#

readpkr.exe: readpkr.cpp ipconn.cpp timeutil.cpp
	@echo Building readpkr.exe
	cl $(cdebug) $(cflags) $(appflags) readpkr.cpp
	$(link) $(ldebug) $(conlflags) -out:readpkr.exe readpkr.obj $(conlibsmt) winmm.lib ws2_32.lib

#
# Read HP 34461A or similar instrument that responds to Telnet-style READ? queries over TCP/IP
# (usually port 5025 is appropriate)
#

readinst.exe: readinst.cpp ipconn.cpp timeutil.cpp appfile.cpp stdtpl.h
	@echo Building readinst.exe
	cl $(cdebug) $(cflags) $(appflags) readinst.cpp
	$(link) $(ldebug) $(conlflags) -out:readinst.exe readinst.obj $(conlibsmt) winmm.lib ws2_32.lib

#
# Test signal generator at random frequencies and amplitudes
#

sgentest.exe: sgentest.cpp gpiblib.h gpiblib.lib
	@echo Building sgentest.exe
	cl $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T sgentest.cpp
	$(link) $(ldebug) $(conlflags) -out:sgentest.exe sgentest.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Program 33004A/33005A attenuators connected to HP 3488A / 44470A switch box
#

atten_3488a.exe: atten_3488a.cpp gpiblib.h gpiblib.lib
	@echo Building atten_3488a.exe
	cl $(cdebug) $(cflags) $(appflags) atten_3488a.cpp
	$(link) $(ldebug) $(conlflags) -out:atten_3488a.exe atten_3488a.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Control relays on 44471A card in HP 3448A chassis
#

44471A.exe: 44471A.cpp gpiblib.h gpiblib.lib
	@echo Building 44471A.exe
	cl $(cdebug) $(cflags) $(appflags) 44471A.cpp
	$(link) $(ldebug) $(conlflags) -out:44471A.exe 44471A.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Set HP 5071A clock to NTP server time
#

5071a.exe: 5071a.cpp timeutil.cpp comport.cpp ipconn.cpp
	@echo Building 5071a.exe
	cl $(cdebug) $(cflags) $(appflags) 5071a.cpp
	$(link) $(ldebug) $(conlflags) -out:5071a.exe 5071a.obj $(conlibsmt) winmm.lib user32.lib

#
# MX20 comms test
#

MX20.exe: MX20.cpp timeutil.cpp comport.cpp
	@echo Building MX20.exe
	cl $(cdebug) $(cflags) $(appflags) MX20.cpp
	$(link) $(ldebug) $(conlflags) -out:MX20.exe MX20.obj $(conlibsmt) winmm.lib user32.lib

#
# Send string to serial port
#

txcom.exe: txcom.cpp timeutil.cpp comport.cpp
	@echo Building txcom.exe
	cl $(cdebug) $(cflags) $(appflags) txcom.cpp
	$(link) $(ldebug) $(conlflags) -out:txcom.exe txcom.obj $(conlibsmt) winmm.lib user32.lib

#
# Read data from T962 oven with ESTechnical controller
#

t962.exe: t962.cpp timeutil.cpp comport.cpp
	@echo Building t962.exe
	cl $(cdebug) $(cflags) $(appflags) t962.cpp
	$(link) $(ldebug) $(conlflags) -out:t962.exe t962.obj $(conlibsmt) winmm.lib user32.lib

#
# Request current frequency from HP 5370A/B
#

5370.exe: 5370.cpp gpiblib.h gpiblib.lib
	@echo Building 5370.exe
	cl $(cdebug) $(cflags) $(appflags) 5370.cpp
	$(link) $(ldebug) $(conlflags) -out:5370.exe 5370.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Request current frequency from HP 5345A
#

5345a.exe: 5345a.cpp gpiblib.h gpiblib.lib
	@echo Building 5345a.exe
	cl $(cdebug) $(cflags) $(appflags) 5345a.cpp
	$(link) $(ldebug) $(conlflags) -out:5345a.exe 5345a.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Set freq/amplitude on HP 8672A
#

8672a.exe: 8672a.cpp gpiblib.h gpiblib.lib
	@echo Building 8672a.exe
	cl $(cdebug) $(cflags) $(appflags) 8672a.cpp
	$(link) $(ldebug) $(conlflags) -out:8672a.exe 8672a.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Control various features in HP 11848a
#

11848a.exe: 11848a.cpp gpiblib.h gpiblib.lib
	@echo Building 11848a.exe
	cl $(cdebug) $(cflags) $(appflags) 11848a.cpp
	$(link) $(ldebug) $(conlflags) -out:11848a.exe 11848a.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Trigger and take readings from Wavecrest DTS 207x digital time analyzer
#

dts2070.exe: dts2070.cpp gpiblib.h gpiblib.lib
	@echo Building dts2070.exe
	cl $(cdebug) $(cflags) $(appflags) dts2070.cpp
	$(link) $(ldebug) $(conlflags) -out:dts2070.exe dts2070.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Acquire .GIF screen dump from Agilent E4406A vector signal analyzer
#

psaplot.exe: psaplot.cpp gpiblib.h gpiblib.lib
	@echo Building psaplot.exe
	cl $(cdebug) $(cflags) $(appflags) psaplot.cpp
	$(link) $(ldebug) $(conlflags) -out:psaplot.exe psaplot.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Trigger and take readings from Agilent DSO/MSO model
# (tested with MSO6054A)
#

dso6000.exe: dso6000.cpp gpiblib.h gpiblib.lib
	@echo Building dso6000.exe
	cl $(cdebug) $(cflags) $(appflags) dso6000.cpp
	$(link) $(ldebug) $(conlflags) -out:dso6000.exe dso6000.obj gpiblib.lib $(conlibsmt) winmm.lib

#
# Back up calibration and data NVRAMs from HP 3458A multimeter
#

hp3458.exe: hp3458.cpp gpiblib.h gpiblib.lib
	@echo Building hp3458.exe
	cl $(cdebug) $(cflags) $(appflags) hp3458.cpp
	$(link) $(ldebug) $(conlflags) -out:hp3458.exe hp3458.obj gpiblib.lib $(conlibsmt)

#
# DLL to fetch raw binary traces from spectrum analyzers
#

specan.obj: specan.cpp spline.cpp specan.h gpiblib.h
		@echo Building specan.obj
		$(cc) $(cdebug) $(cflags) specan.cpp

specan.lib: specan.obj gpiblib.lib
		@echo Building specan.lib
		$(link) $(ldebug) $(dlllflags) $(guilibsmt) specan.obj sh_api.lib gpiblib.lib kernel32.lib user32.lib winmm.lib

#
# DLL to fetch fix information from various GNSS receivers (NMEA GPS, etc.)
#

gnss.obj: gnss.cpp gnss.h gpiblib\comblock.h
		@echo Building gnss.obj
		$(cc) $(cdebug) $(cflags) gnss.cpp

gnss.lib: gnss.obj gpiblib.lib
		@echo Building gnss.lib
		$(link) $(ldebug) $(dlllflags) $(guilibsmt) gnss.obj kernel32.lib user32.lib winmm.lib

#
# Display GNSS fixes from GPS or other GNSS sources
#

gps.exe: gps.cpp gnss.h gnss.lib
	@echo Building gps.exe
	cl $(cdebug) $(cflags) $(appflags) gps.cpp
	$(link) $(ldebug) $(conlflags) -out:gps.exe gps.obj gnss.lib $(conlibsmt) winmm.lib

#
# Request .CSV traces from spectrum analyzers
#

satrace.exe: satrace.cpp specan.h specan.lib
	@echo Building satrace.exe
	cl $(cdebug) $(cflags) $(appflags) satrace.cpp
	$(link) $(ldebug) $(conlflags) -out:satrace.exe satrace.obj specan.lib $(conlibsmt) winmm.lib

#
# Fetch and scale data from a DataQ DI-154RS or DI-194RS serial DAQ unit
#

dataq.exe: dataq.cpp timeutil.cpp comport.cpp di154.cpp
	@echo Building dataq.exe
	cl $(cdebug) $(cflags) $(appflags) dataq.cpp
	$(link) $(ldebug) $(conlflags) -out:dataq.exe dataq.obj $(conlibsmt) winmm.lib user32.lib

#
# Stream readings from an HP 34461A or similar VISA-enabled DMM
#

visadmm.exe: visadmm.cpp timeutil.cpp
	@echo Building visadmm.exe
	cl $(cdebug) $(cflags) $(appflags) visadmm.cpp
	$(link) $(ldebug) $(conlflags) -out:visadmm.exe visadmm.obj agvisa32.lib $(conlibsmt) winmm.lib user32.lib

#
# Generic test app
#

t.exe: t.cpp timeutil.cpp
	@echo Building t.exe
	cl $(cdebug) $(cflags) $(appflags) t.cpp
	$(link) $(ldebug) $(conlflags) -out:t.exe t.obj $(conlibsmt) winmm.lib user32.lib

#
# Surveillance monitor program and display components
#

ssm.res: ssm.ico ssm.rc ssmres.h
   rc ssm.rc

ssm.exe: ssm.obj xy.obj waterfall.obj recorder.obj winvfx16.lib w32sal.lib specan.lib gnss.lib ssm.res
	@echo Building ssm.exe
	$(link) $(ldebug) $(guilflags) -out:ssm.exe ssm.obj xy.obj waterfall.obj recorder.obj w32sal.lib winvfx16.lib specan.lib gnss.lib $(guilibsmt) winmm.lib shell32.lib ssm.res

ssmdump.exe: ssmdump.obj recorder.obj gnss.lib
	@echo Building ssmdump.exe
	$(link) $(ldebug) $(conlflags) -out:ssmdump.exe ssmdump.obj recorder.obj gnss.lib $(conlibsmt) winmm.lib

ssmchan.exe: ssmchan.obj recorder.obj gnss.lib
	@echo Building ssmchan.exe
	$(link) $(ldebug) $(conlflags) -out:ssmchan.exe ssmchan.obj recorder.obj gnss.lib $(conlibsmt) winmm.lib

xy.obj: xy.h xy.cpp winvfx.h w32sal.h
   $(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T xy.cpp

waterfall.obj: waterfall.h waterfall.cpp winvfx.h w32sal.h
   $(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T waterfall.cpp

recorder.obj: recorder.h recorder.cpp winvfx.h w32sal.h
   $(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T recorder.cpp

ssm.obj: ssm.cpp w32sal.h winvfx.h specan.h gnss.h xy.h waterfall.h recorder.h ssmres.h
    $(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T ssm.cpp

ssmdump.obj: ssmdump.cpp recorder.h
    $(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T ssmdump.cpp

ssmchan.obj: ssmchan.cpp recorder.h
    $(cc) $(cdebug) $(cflags) $(appflags) /D_USE_32BIT_TIME_T ssmchan.cpp

#
# VNA performance test
#

tcheck.res: tcheck.ico tcheck.rc tchkres.h
   rc tcheck.rc

tcheck.exe: tcheck.obj xy.obj winvfx16.lib w32sal.lib tcheck.res
	@echo Building tcheck.exe
#   $(link) $(ldebug) $(guilflags) -out:tcheck.exe tcheck.obj xy.obj w32sal.lib winvfx16.lib $(guilibsmt) /NODEFAULTLIB:libcmtd.lib winmm.lib shell32.lib tcheck.res
	$(link) $(ldebug) $(guilflags) -out:tcheck.exe tcheck.obj xy.obj w32sal.lib winvfx16.lib $(guilibsmt) winmm.lib shell32.lib tcheck.res

tcheck.obj: tcheck.cpp spline.cpp sparams.cpp w32sal.h winvfx.h xy.h tchkres.h
#   $(cc) $(crelease) $(cflags) $(appflags) tcheck.cpp
   $(cc) $(cdebug) $(cflags) $(appflags) tcheck.cpp

#
# Prologix/CONNECT.INI configurator
#

prologix.res: gpib.ico prologix.rc prores.h
   rc prologix.rc

prologix.exe: prologix.obj prologix.res
	@echo Building prologix.exe
	$(link) $(ldebug) $(guilflags) -out:prologix.exe prologix.obj gpiblib/gpib-32.obj $(guilibsmt) winmm.lib shell32.lib comctl32.lib ws2_32.lib setupapi.lib prologix.res

prologix.obj: prologix.cpp typedefs.h prores.h gpiblib\comblock.h
    $(cc) $(cdebug) $(cflags) $(appflags) prologix.cpp

#
# HP 8510/8753-series VNA state/calibration maintenance utility
#

vna.res: vna.ico vna.rc vnares.h
   rc vna.rc

vna.exe: vna.obj gpiblib.lib vna.res
	@echo Building vna.exe
	$(link) $(ldebug) $(guilflags) -out:vna.exe vna.obj gpiblib.lib $(guilibsmt) winmm.lib comctl32.lib vna.res

vna.obj: vna.cpp sparams.cpp spline.cpp typedefs.h gpiblib.h vnares.h
    $(cc) $(cdebug) $(cflags) $(appflags) vna.cpp

#
# TCP/IP loopback benchmark/diagnostic
#

echoserver.exe: echoserver.cpp
	@echo Building echoserver.exe
	cl -EHsc -D_CRT_SECURE_NO_DEPRECATE echoserver.cpp user32.lib ws2_32.lib

echoclient.exe: echoclient.cpp gpiblib/comblock.h
	@echo Building echoclient.exe
	cl $(cdebug) $(cflags) $(appflags) echoclient.cpp
	$(link) $(ldebug) $(conlflags) -out:echoclient.exe echoclient.obj $(conlibsmt) ws2_32.lib winmm.lib user32.lib

#
# Other tests
#

mtie.exe: mtie.cpp
	@echo Building mtie.exe
	cl -EHsc mtie.cpp user32.lib

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
    del *.exe *.obj *.res specan.lib gnss.lib 7470.ini pn.ini
