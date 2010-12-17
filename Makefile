#CFLAGS=-Wall -g -ansi -fPIC
CFLAGS=-Wall -O2 -ansi -fPIC
LIB_TAG=0.1.10
CC=gcc
PREFIX=/usr/local

all: sonic libsonic.so.$(LIB_TAG)

sonic: wave.o main.o libsonic.so.$(LIB_TAG)
	$(CC) $(CFLAGS) -lsndfile libsonic.so.$(LIB_TAG) -o sonic wave.o main.o

sonic.o: sonic.c sonic.h
	$(CC) $(CFLAGS) -c sonic.c

wave.o: wave.c wave.h
	$(CC) $(CFLAGS) -c wave.c

main.o: main.c sonic.h wave.h
	$(CC) $(CFLAGS) -c main.c

libsonic.so.$(LIB_TAG): sonic.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,libsonic.so.0 sonic.o -o libsonic.so.$(LIB_TAG)
	ln -sf libsonic.so.$(LIB_TAG) libsonic.so

install: sonic libsonic.so.$(LIB_TAG) sonic.h
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install sonic $(DESTDIR)$(PREFIX)/bin
	install sonic.h $(DESTDIR)$(PREFIX)/include
	install libsonic.so.$(LIB_TAG) $(DESTDIR)$(PREFIX)/lib
	ln -sf libsonic.so.$(LIB_TAG) $(DESTDIR)$(PREFIX)/lib/libsonic.so

clean:
	rm -f *.o sonic libsonic.so*
