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

/* interpolate is not declared static in sonic.c specifically so this
   white-box test can call it directly, without needing to drive the
   public streaming API through the exact internal state (stream
   position counters, sample-rate scaling) that reproducing this
   deterministically would otherwise require. It is not part of the
   public sonic.h contract. */
short interpolate(sonicStream stream, short* in, int oldSampleRate,
                  int newSampleRate);

/* Verify interpolate's 12-tap sinc-filter accumulator clamps correctly
   instead of relying on signed integer overflow to detect that it needs
   to. On a freshly created stream, newRatePosition and oldRatePosition
   are both 0, which makes interpolate's internal ratio/width depend only
   on newSampleRate -- so choosing newSampleRate = 8000 gives a known,
   reproducible weight at each of the 12 taps (only tap 5 has a large
   coefficient; the filter is centered almost exactly on that tap for
   this ratio). Setting in[] to the maximum-magnitude sample matching
   each tap's coefficient sign guarantees the accumulator's true sum
   exceeds INT_MAX -- this exact configuration was verified via UBSan
   during development to trip a signed-integer-overflow at the old,
   unfixed accumulation step, and to no longer do so after the fix,
   while still returning the same, correctly clamped SHRT_MAX. */
int sonicTestInterpolateOverflowClamping(void) {
    sonicStream stream;
    short in[12] = {32767, 32767, -32768, 32767, -32768, 32767,
                     32767, -32768, 32767, 32767, 32767, 32767};
    short result;

    stream = sonicCreateStream(SAMPLE_RATE, NUM_CHANNELS);
    if (stream == NULL) {
        fprintf(stderr, "sonicCreateStream failed\n");
        return 0;
    }
    result = interpolate(stream, in, SAMPLE_RATE, 8000);
    sonicDestroyStream(stream);
    if (result != SHRT_MAX) {
        fprintf(stderr,
                "interpolate should clamp this overflowing sum to SHRT_MAX, got %d\n",
                result);
        return 0;
    }
    return 1;
}
