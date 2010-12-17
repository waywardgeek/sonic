#CFLAGS=-Wall -g -ansi -fPIC
CFLAGS=-Wall -O2 -ansi -fPIC
LIB_TAG=0.1.8
CC=gcc
PREFIX=/usr/local

all: sonic libsonic0.so.$(LIB_TAG)

sonic: wave.o main.o libsonic0.so.$(LIB_TAG)
	$(CC) $(CFLAGS) -lsndfile libsonic0.so.$(LIB_TAG) -o sonic wave.o main.o

sonic.o: sonic.c sonic.h
	$(CC) $(CFLAGS) -c sonic.c

wave.o: wave.c wave.h
	$(CC) $(CFLAGS) -c wave.c

main.o: main.c sonic.h wave.h
	$(CC) $(CFLAGS) -c main.c

libsonic0.so.$(LIB_TAG): sonic.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,libsonic0.so.0 sonic.o -o libsonic0.so.$(LIB_TAG)
	ln -sf libsonic0.so.$(LIB_TAG) libsonic0.so.0

install: sonic libsonic0.so.$(LIB_TAG) sonic.h
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install sonic $(DESTDIR)$(PREFIX)/bin
	install sonic.h $(DESTDIR)$(PREFIX)/include
	install libsonic0.so.$(LIB_TAG) $(DESTDIR)$(PREFIX)/lib
	ln -sf libsonic0.so.$(LIB_TAG) $(DESTDIR)$(PREFIX)/lib/libsonic0.so.0

clean:
	rm -f *.o sonic libsonic0.so*
