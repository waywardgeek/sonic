#CFLAGS=-Wall -g -ansi -fPIC
CFLAGS=-Wall -O2 -ansi -fPIC
LIB_TAG=0.1.8

all: sonic libsonic.so.$(LIB_TAG)

sonic: wave.o main.o libsonic.so.$(LIB_TAG)
	gcc $(CFLAGS) -lsndfile libsonic.so.$(LIB_TAG) -o sonic wave.o main.o

sonic.o: sonic.c sonic.h
	gcc $(CFLAGS) -c sonic.c

wave.o: wave.c wave.h
	gcc $(CFLAGS) -c wave.c

main.o: main.c sonic.h wave.h
	gcc $(CFLAGS) -c main.c

libsonic.so.$(LIB_TAG): sonic.o
	gcc $(CFLAGS) -shared -Wl,-soname,libsonic.so.0 sonic.o -o libsonic.so.$(LIB_TAG)
	ln -sf libsonic.so.$(LIB_TAG) libsonic.so.0

install: sonic libsonic.so.$(LIB_TAG) sonic.h
	install -d $(DESTDIR)/usr/bin $(DESTDIR)/usr/include $(DESTDIR)/usr/lib
	install sonic $(DESTDIR)/usr/bin
	install sonic.h $(DESTDIR)/usr/include
	install libsonic.so.$(LIB_TAG) $(DESTDIR)/usr/lib
	ln -sf libsonic.so.$(LIB_TAG) $(DESTDIR)/usr/lib/libsonic.so.0

clean:
	rm -f *.o sonic libsonic.so*
