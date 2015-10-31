# Generated automatically from Makefile.in by configure.
.SUFFIXES: .c .o
SHELL = /bin/sh


subdirs = @subdirs@
top_srcdir = .
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info
libdir = $(prefix)/lib/gnudl
mandir = $(prefix)/man/man1

CC = gcc
CFLAGS = $(CPPFLAGS) -g -O2
LIBS =  -lm

all:
	$(CC) $(CFLAGS) -o xbiso xbiso.c $(LIBS)
compress:
	strip xbiso
	upx -9 xbiso>/dev/null
clean:
	rm -f xbiso config.cache config.log config.status config.h Makefile

install:
	install -m 755 xbiso $(bindir)

uninstall:
	rm -f $(bindir)/xbiso
