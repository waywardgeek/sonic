#CFLAGS=-Wall -g -ansi
CFLAGS=-Wall -O2 -ansi -fPIC

all: sonic libsonic.so

sonic: wave.o main.o libsonic.so
	gcc $(CFLAGS) -lsndfile -L. -lsonic -o sonic wave.o main.o

sonic.o: sonic.c sonic.h
	gcc $(CFLAGS) -c sonic.c

wave.o: wave.c wave.h
	gcc $(CFLAGS) -c wave.c

main.o: main.c sonic.h wave.h
	gcc $(CFLAGS) -c main.c

libsonic.so: sonic.o
	gcc $(CFLAGS) -shared sonic.o -o libsonic.so 

install: sonic libsonic.so sonic.h
	install -d $(DESTDIR)/usr/bin $(DESTDIR)/usr/include $(DESTDIR)/usr/lib
	install sonic $(DESTDIR)/usr/bin
	install sonic.h $(DESTDIR)/usr/include
	install libsonic.so $(DESTDIR)/usr/lib

clean:
	rm *.o
