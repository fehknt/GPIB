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
         -D_MT -D_CRT_SECURE_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS

link = link
ldebug = /DEBUG /DEBUGTYPE:cv
guilflags = /INCREMENTAL:NO /NOLOGO -subsystem:windows,4.10
conlflags = /INCREMENTAL:NO /NOLOGO -subsystem:console,4.10
dlllflags = /INCREMENTAL:NO /NOLOGO -subsystem:windows,4.10 -entry:_DllMainCRTStartup@12 -dll
guilibsmt = kernel32.lib ws2_32.lib mswsock.lib advapi32.lib user32.lib gdi32.lib comdlg32.lib
conlibsmt = kernel32.lib ws2_32.lib mswsock.lib advapi32.lib user32.lib

#
# Targets
#

OBJDIR = obj
BINDIR = bin

all: dirs $(BINDIR)\gpiblib.dll

dirs:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(BINDIR) mkdir $(BINDIR)

$(OBJDIR)\gpiblib.obj: gpiblib.cpp decl-32.h gpiblib.h comblock.h
	$(cc) $(cdebug) $(cflags) /Fo$@ gpiblib.cpp

$(BINDIR)\gpiblib.dll: $(OBJDIR)\gpiblib.obj gpib-32.obj
	$(link) $(ldebug) $(dlllflags) $(guilibsmt) $(OBJDIR)\gpiblib.obj gpib-32.obj winmm.lib kernel32.lib user32.lib shell32.lib /OUT:$@

#
# Optional standalone console app for test development, not built by default
#

$(BINDIR)\gpiblib.exe: $(OBJDIR)\gpiblib.obj gpib-32.obj
	$(link) $(ldebug) $(conflags) $(guilibsmt) $(OBJDIR)\gpiblib.obj gpib-32.obj winmm.lib kernel32.lib user32.lib shell32.lib /OUT:$@
