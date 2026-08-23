/* Sonic library
   Copyright 2026
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#include "sonic.h"

#include "genwave.h"

#include <stdio.h>

#define SAMPLE_RATE 96000
#define FREQ 200
#define PERIOD (SAMPLE_RATE / FREQ)
#define AMPLITUDE 6000
#define CHUNK_PERIODS 3
#define CHUNK_SAMPLES (CHUNK_PERIODS * PERIOD)
#define READ_BUF_LEN 4096

/* sonicSetRate resets oldRatePosition/newRatePosition to 0 (see its
   definition just above sonicSetPitch), because adjustRate's wraparound
   arithmetic assumes those two counters were accumulated under a single,
   consistent (oldSampleRate, newSampleRate) ratio -- one that changing the
   rate mid-stream immediately invalidates. processStreamInput folds pitch
   into that same ratio (rate = stream->rate * stream->pitch), so
   sonicSetPitch invalidates the ratio exactly the same way, but it doesn't
   reset the counters. Left to drift across enough pitch changes, the
   counters grow large enough to integer-overflow interpolate()'s position
   arithmetic, indexing sincTable wildly out of bounds -- confirmed via
   ASan (global-buffer-overflow in findSincCoefficient) and a native crash
   without it. This specific pitch sequence and sample rate were found by
   sweeping random pitch changes for a case that crashes reliably and then
   trimmed to the smallest deterministic repro. */
int sonicTestRepeatedPitchChangesDontDesyncRatePosition(void) {
    short chunk[CHUNK_SAMPLES];
    sonicStream stream;
    short readBuf[READ_BUF_LEN];
    float pitches[] = {2.5f, 0.4f, 3.1f, 0.6f, 4.0f, 0.3f, 2.0f, 0.5f, 3.5f, 0.45f};
    int i, n = sizeof(pitches) / sizeof(pitches[0]);
    int actualSamples;

    stream = sonicCreateStream(SAMPLE_RATE, 1);
    if (stream == NULL) {
        fprintf(stderr, "sonicCreateStream failed\n");
        return 0;
    }
    for (i = 0; i < n; i++) {
        actualSamples = genSineWave(chunk, CHUNK_SAMPLES, SAMPLE_RATE, PERIOD,
                                    AMPLITUDE, CHUNK_PERIODS);
        sonicSetPitch(stream, pitches[i]);
        if (!sonicWriteShortToStream(stream, chunk, actualSamples)) {
            fprintf(stderr, "sonicWriteShortToStream failed\n");
            sonicDestroyStream(stream);
            return 0;
        }
        while (sonicReadShortFromStream(stream, readBuf, READ_BUF_LEN) != 0);
    }
    sonicFlushStream(stream);
    while (sonicReadShortFromStream(stream, readBuf, READ_BUF_LEN) != 0);
    sonicDestroyStream(stream);
    return 1;
}
