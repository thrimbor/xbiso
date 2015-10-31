# Wedit Makefile for project xbiso-win32
SRCDIR=d:\lcc\projects\xbiso-0.5.6
CFLAGS=-Id:\lcc\include  -O  -p6
CC=d:\lcc\bin\lcc.exe
LINKER=d:\lcc\bin\lcclnk.exe
OBJS=\
	xbiso.obj \
	getopt.obj \
	alloca.obj

LIBS= tcconio.lib

xbiso-win32.exe:	$(OBJS) Makefile
	$(LINKER)  -subsystem console -o $(SRCDIR)\lcc\xbiso-win32.exe $(OBJS) $(LIBS)

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

# Build ALLOCA.C
ALLOCA_C=\

alloca.obj: $(ALLOCA_C) $(SRCDIR)\alloca.c
	$(CC) -c $(CFLAGS) $(SRCDIR)\alloca.c

link:
	$(LINKER)  -subsystem console -o $(SRCDIR)\lcc\xbiso-win32.exe $(OBJS) $(LIBS)

