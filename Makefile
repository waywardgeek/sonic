# This file was written by Bill Cox in 2010, and is licensed under the Apache
# 2.0 license.
#
# Note that -pthread is only included so that older Linux builds will be thread
# safe.  We call malloc, and older Linux versions only linked in the thread-safe
# malloc if -pthread is specified.

# Uncomment this if you want to link in spectrogram generation.  It is not
# needed to adjust speech speed or pitch.  It is included primarily to provide
# high-quality spectrograms with low CPU overhead, for applications such a
# speech recognition.
#USE_SPECTROGRAM=1

PREFIX=/usr

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
  PREFIX=/usr/local
endif

BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

SONAME=-soname,
SHARED_OPT=-shared
LIB_NAME=libsonic.so
LIB_TAG=.0.3.0

ifeq ($(UNAME), Darwin)
  SONAME=-install_name,$(LIBDIR)/
  SHARED_OPT=-dynamiclib
  LIB_NAME=libsonic.dylib
  LIB_TAG=
endif

#CFLAGS=-Wall -Wno-unused-function -g -ansi -fPIC -pthread
CFLAGS ?= -O3
CFLAGS += -Wall -Wno-unused-function -ansi -fPIC -pthread

CC=gcc

SRC=sonic.c
# Set this to empty if not using spectrograms.
FFTLIB=
ifeq ($(USE_SPECTROGRAM), 1)
  CFLAGS+= -DSONIC_SPECTROGRAM
  SRC+= spectrogram.c
  FFTLIB= -L$(LIBDIR) -lfftw3
endif
OBJ=$(SRC:.c=.o)

all: sonic $(LIB_NAME)$(LIB_TAG) libsonic.a

sonic: wave.o main.o libsonic.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o sonic wave.o main.o libsonic.a -lm $(FFTLIB)

sonic.o: sonic.c sonic.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c sonic.c

wave.o: wave.c wave.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c wave.c

main.o: main.c sonic.h wave.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c main.c

spectrogram.o: spectrogram.c sonic.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c spectrogram.c

$(LIB_NAME)$(LIB_TAG): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SHARED_OPT) -Wl,$(SONAME)$(LIB_NAME) $(OBJ) -o $(LIB_NAME)$(LIB_TAG) $(FFTLIB)
ifneq ($(UNAME), Darwin)
	ln -sf $(LIB_NAME)$(LIB_TAG) $(LIB_NAME)
	ln -sf $(LIB_NAME)$(LIB_TAG) $(LIB_NAME).0
endif

libsonic.a: $(OBJ)
	$(AR) cqs libsonic.a $(OBJ)

install: sonic $(LIB_NAME)$(LIB_TAG) sonic.h
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(INCDIR) $(DESTDIR)$(LIBDIR)
	install sonic $(DESTDIR)$(BINDIR)
	install sonic.h $(DESTDIR)$(INCDIR)
	install libsonic.a $(DESTDIR)$(LIBDIR)
	install $(LIB_NAME)$(LIB_TAG) $(DESTDIR)$(LIBDIR)
ifneq ($(UNAME), Darwin)
	ln -sf $(LIB_NAME)$(LIB_TAG) $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
	ln -sf $(LIB_NAME)$(LIB_TAG) $(DESTDIR)$(LIBDIR)/$(LIB_NAME).0
endif

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/sonic
	rm -f $(DESTDIR)$(INCDIR)/sonic.h
	rm -f $(DESTDIR)$(LIBDIR)/libsonic.a
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME)$(LIB_TAG)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME).0
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME)

clean:
	rm -f *.o sonic $(LIB_NAME)* libsonic.a test.wav

check:
	./sonic -s 2.0 ./samples/talking.wav ./test.wav
