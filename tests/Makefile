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

CFLAGS=-Wall -Wno-unused-function -g -ansi -fPIC -pthread -I ..
#CFLAGS ?= -O3
#CFLAGS += -Wall -Wno-unused-function -ansi -fPIC -pthread -I ..

TEST_SRC = \
input_clamping_test.c

CC=gcc

all: genwave runtests

genwave: ../wave.c ../wave.h genwave.c genwave.h genwave_main.c
	$(CC) $(CFLAGS) -o genwave genwave.c genwave_main.c ../wave.c -lm

runtests: runtests.c genwave.c ../sonic.c ../sonic.h tests.h $(TEST_SRC)
	$(CC) $(CFLAGS) -o runtests runtests.c genwave.c ../sonic.c $(TEST_SRC) -lm

clean:
	rm -f *.o genwave runtests
