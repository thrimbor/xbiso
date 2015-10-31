.SUFFIXES: .c .o
CC=gcc

xbiso:
	gcc -O2 -o xbiso xbiso.c
compress:
	strip xbiso
	upx -9 xbiso>/dev/null
clean:
	rm -f xbiso

