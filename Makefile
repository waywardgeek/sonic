CFLAGS=-Wall -g -c -ansi
#CFLAGS=-Wall -O2 -c -ansi
LFLAGS= -Wall -g -ansi -lsndfile

sonic: sonic.o wave.o main.o
	gcc $(LFLAGS) -o sonic sonic.o wave.o main.o

sonic.o: sonic.c sonic.h
	gcc $(CFLAGS) sonic.c

wave.o: wave.c wave.h
	gcc $(CFLAGS) wave.c

main.o: main.c sonic.h wave.h
	gcc $(CFLAGS) main.c

clean:
	rm *.o
