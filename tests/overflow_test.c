/* Sonic library
   Copyright 2026
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#include "sonic.h"

#include <limits.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1

/* enlargeOutputBufferIfNeeded and insertPitchPeriod are not declared static
   in sonic.c specifically so these white-box tests can call them directly,
   without going through the public streaming API to reach the overflow
   paths. They are not part of the public sonic.h contract. */
int enlargeOutputBufferIfNeeded(sonicStream stream, int numSamples);
int insertPitchPeriod(sonicStream stream, short* samples, float speed,
                      int period);

int sonicTestEnlargeOutputBufferRejectsOverflow(void) {
    sonicStream stream;
    int result;

    stream = sonicCreateStream(SAMPLE_RATE, NUM_CHANNELS);
    if (stream == NULL) {
        fprintf(stderr, "sonicCreateStream failed\n");
        return 0;
    }
    /* A request this large can't be satisfied; the guard must fail cleanly
       instead of signed-overflowing the internal buffer size arithmetic. */
    result = enlargeOutputBufferIfNeeded(stream, INT_MAX);
    sonicDestroyStream(stream);
    if (result != 0) {
        fprintf(stderr,
                "enlargeOutputBufferIfNeeded should reject an overflowing request\n");
        return 0;
    }
    return 1;
}

int sonicTestEnlargeOutputBufferAcceptsNormalRequest(void) {
    sonicStream stream;
    int result;

    stream = sonicCreateStream(SAMPLE_RATE, NUM_CHANNELS);
    if (stream == NULL) {
        fprintf(stderr, "sonicCreateStream failed\n");
        return 0;
    }
    result = enlargeOutputBufferIfNeeded(stream, 128);
    sonicDestroyStream(stream);
    if (result != 1) {
        fprintf(stderr,
                "enlargeOutputBufferIfNeeded should accept a normal request\n");
        return 0;
    }
    return 1;
}

int sonicTestInsertPitchPeriodRejectsOverflow(void) {
    sonicStream stream;
    short dummy[1];
    int result;

    stream = sonicCreateStream(SAMPLE_RATE, NUM_CHANNELS);
    if (stream == NULL) {
        fprintf(stderr, "sonicCreateStream failed\n");
        return 0;
    }
    dummy[0] = 0;
    /* period this large makes period + newSamples overflow INT_MAX before
       the fix, which used to truncate silently at the
       enlargeOutputBufferIfNeeded call boundary. The function must return
       failure before touching samples, so a 1-element buffer is safe here. */
    result = insertPitchPeriod(stream, dummy, 1.0f, INT_MAX);
    sonicDestroyStream(stream);
    if (result != 0) {
        fprintf(stderr,
                "insertPitchPeriod should reject an overflowing period\n");
        return 0;
    }
    return 1;
}
