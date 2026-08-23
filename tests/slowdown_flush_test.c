/* Sonic library
   Copyright 2026
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#include "sonic.h"

#include "genwave.h"

#include <stdio.h>

#define SAMPLE_RATE 44100
#define FREQ 180
#define PERIOD (SAMPLE_RATE / FREQ)
#define AMPLITUDE 6000
#define NUM_PERIODS 10
#define NUM_SAMPLES (NUM_PERIODS * PERIOD)
#define SPEED 0.060000002f
#define READ_BUF_LEN 100000

/* https://github.com/waywardgeek/sonic/issues/38: a short sample slowed
   down heavily (speed <= 0.5, the insertPitchPeriod path) came out both
   too short overall and padded with a large block of trailing silence.

   sonicFlushStream appends 2*maxRequired samples of silence directly to
   numInputSamples (to give the pitch-period algorithms enough lookahead
   to drain what's buffered) but never advances inputPlayTime to match.
   processStreamInput computes its actual working speed as
   numInputSamples * samplePeriod / inputPlayTime -- so once flush's
   padding inflates numInputSamples without inputPlayTime keeping pace,
   that computed speed comes out much higher than the caller's requested
   speed, and insertPitchPeriod stretches the remaining audio by far less
   than it should. The last chunk of real audio and the trailing silence
   both get compressed into a much shorter span than expected, and
   whatever's left over reads back as silence. */
int sonicTestSlowdownFlushProducesExpectedDuration(void) {
    short samples[NUM_SAMPLES];
    int numSamples = genSineWave(samples, NUM_SAMPLES, SAMPLE_RATE, PERIOD,
                                 AMPLITUDE, NUM_PERIODS);
    sonicStream stream;
    short output[READ_BUF_LEN];
    int total = 0, samplesRead, nonzeroEnd;
    double expected;

    if (numSamples != NUM_SAMPLES) {
        fprintf(stderr, "genSineWave failed\n");
        return 0;
    }
    stream = sonicCreateStream(SAMPLE_RATE, 1);
    if (stream == NULL) {
        fprintf(stderr, "sonicCreateStream failed\n");
        return 0;
    }
    sonicSetSpeed(stream, SPEED);
    if (!sonicWriteShortToStream(stream, samples, numSamples)) {
        fprintf(stderr, "sonicWriteShortToStream failed\n");
        sonicDestroyStream(stream);
        return 0;
    }
    sonicFlushStream(stream);
    while ((samplesRead = sonicReadShortFromStream(stream, output,
                                                    READ_BUF_LEN)) != 0) {
        total += samplesRead;
    }
    sonicDestroyStream(stream);

    expected = numSamples / (double)SPEED;
    if (total < 0.9 * expected || total > 1.1 * expected) {
        fprintf(stderr,
                "expected total output near %f samples, got %d\n",
                expected, total);
        return 0;
    }

    nonzeroEnd = total;
    while (nonzeroEnd > 0 && output[nonzeroEnd - 1] == 0) {
        nonzeroEnd--;
    }
    if (total - nonzeroEnd > 0.05 * total) {
        fprintf(stderr,
                "trailing silence too large: %d of %d output samples (%.1f%%)\n",
                total - nonzeroEnd, total, 100.0 * (total - nonzeroEnd) / total);
        return 0;
    }
    return 1;
}
