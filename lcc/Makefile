# Wedit Makefile for project xbiso057
SRCDIR=g:\xbiso-0.5.7
CFLAGS=-Id:\lcc\include -D_WIN32  -O  -p6
CC=$(LCCROOT)\bin\lcc.exe
LINKER=$(LCCROOT)\bin\lcclnk.exe
OBJS=\
	xbiso.obj \
	getopt.obj

LIBS= tcconio.lib
EXE=xbiso057.exe

$(EXE):	$(OBJS) Makefile
	$(LINKER)  -s -subsystem console -o $(SRCDIR)\lcc\xbiso057.exe $(OBJS) $(LIBS)

# Build XBISO.C
XBISO_C=\
	$(SRCDIR)\getopt.h\
	$(SRCDIR)\xbiso.h\

xbiso.obj: $(XBISO_C) $(SRCDIR)\xbiso.c
	$(CC) -c $(CFLAGS) $(SRCDIR)\xbiso.c

# Build GETOPT.C
GETOPT_C=\
	$(SRCDIR)\getopt.h\
	$(SRCDIR)\xbisowin32.h\

getopt.obj: $(GETOPT_C) $(SRCDIR)\getopt.c
	$(CC) -c $(CFLAGS) $(SRCDIR)\getopt.c

link:
	$(LINKER)  -s -subsystem console -o $(SRCDIR)\lcc\xbiso057.exe $(OBJS) $(LIBS)

clean:
	del $(OBJS) g:\xbiso-0.5.7\lcc\xbiso057.exe
