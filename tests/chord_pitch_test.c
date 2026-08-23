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
#define FREQ 200
#define PERIOD (SAMPLE_RATE / FREQ)
#define AMPLITUDE 6000
#define NUM_PERIODS 500
#define NUM_SAMPLES (NUM_PERIODS * PERIOD)
#define READ_BUF_LEN 1000

/* Process all of samples through stream, and return a (length, checksum)
   summary of everything read back, cheap enough to compare two runs without
   holding the whole output in memory. */
static void processAndSummarize(sonicStream stream, const short* samples,
                                int numSamples, int* outLength,
                                unsigned long* outChecksum) {
    short readBuf[READ_BUF_LEN];
    int samplesRead, i;
    int length = 0;
    unsigned long checksum = 0;

    if (!sonicWriteShortToStream(stream, samples, numSamples)) {
        fprintf(stderr, "sonicWriteShortToStream failed\n");
    }
    while ((samplesRead = sonicReadShortFromStream(stream, readBuf,
                                                    READ_BUF_LEN)) != 0) {
        for (i = 0; i < samplesRead; i++) {
            checksum = checksum * 31 + readBuf[i];
        }
        length += samplesRead;
    }
    sonicFlushStream(stream);
    while ((samplesRead = sonicReadShortFromStream(stream, readBuf,
                                                    READ_BUF_LEN)) != 0) {
        for (i = 0; i < samplesRead; i++) {
            checksum = checksum * 31 + readBuf[i];
        }
        length += samplesRead;
    }
    *outLength = length;
    *outChecksum = checksum;
}

/* sonicSetChordPitch's vocal-chord-emulation ("linear") pitch-scaling mode is
   documented to change how pitch is computed (sonic.h: "Set the vocal chord
   mode for pitch computation"). Enabling it must therefore produce different
   output than leaving it off, for the same pitch setting -- otherwise the
   setting has no effect, which is exactly what
   https://github.com/waywardgeek/sonic/issues/49 reports ("sonic -c -p 0.5
   ..." and "sonic -p 0.5 ..." give identical output). */
int sonicTestChordPitchChangesOutput(void) {
    short samples[NUM_SAMPLES];
    int numSamples = genSineWave(samples, NUM_SAMPLES, SAMPLE_RATE, PERIOD,
                                 AMPLITUDE, NUM_PERIODS);
    sonicStream stream;
    int defaultLength, chordLength;
    unsigned long defaultChecksum, chordChecksum;

    if (numSamples != NUM_SAMPLES) {
        fprintf(stderr, "genSineWave failed\n");
        return 0;
    }

    stream = sonicCreateStream(SAMPLE_RATE, 1);
    sonicSetPitch(stream, 0.5f);
    processAndSummarize(stream, samples, numSamples, &defaultLength,
                        &defaultChecksum);
    sonicDestroyStream(stream);

    stream = sonicCreateStream(SAMPLE_RATE, 1);
    sonicSetChordPitch(stream, 1);
    sonicSetPitch(stream, 0.5f);
    processAndSummarize(stream, samples, numSamples, &chordLength,
                        &chordChecksum);
    sonicDestroyStream(stream);

    if (defaultLength == chordLength && defaultChecksum == chordChecksum) {
        fprintf(stderr,
                "sonicSetChordPitch(stream, 1) should change output for the "
                "same pitch setting, but it had no effect (length=%d, "
                "checksum=%lu both times)\n",
                defaultLength, defaultChecksum);
        return 0;
    }
    return 1;
}

/* adjustPitch's newPeriod (period / pitch) is unbounded above and below: at
   SONIC_MIN_PITCH_SETTING, newPeriod balloons far past what's actually
   buffered ahead of the current position, and overlapAddWithSeparation reads
   that many samples from the pitch buffer -- a heap-buffer-overflow,
   confirmed via ASan during development. Just processing audio at the
   extreme ends of the pitch range with chord pitch enabled, without
   crashing, is the regression check here; the exact clamped output isn't
   otherwise meaningful. */
int sonicTestChordPitchExtremeRatioDoesNotOverflow(void) {
    short samples[NUM_SAMPLES];
    int numSamples = genSineWave(samples, NUM_SAMPLES, SAMPLE_RATE, PERIOD,
                                 AMPLITUDE, NUM_PERIODS);
    sonicStream stream;
    int length;
    unsigned long checksum;

    if (numSamples != NUM_SAMPLES) {
        fprintf(stderr, "genSineWave failed\n");
        return 0;
    }

    stream = sonicCreateStream(SAMPLE_RATE, 1);
    sonicSetChordPitch(stream, 1);
    sonicSetPitch(stream, SONIC_MIN_PITCH_SETTING);
    processAndSummarize(stream, samples, numSamples, &length, &checksum);
    sonicDestroyStream(stream);

    stream = sonicCreateStream(SAMPLE_RATE, 1);
    sonicSetChordPitch(stream, 1);
    sonicSetPitch(stream, SONIC_MAX_PITCH_SETTING);
    processAndSummarize(stream, samples, numSamples, &length, &checksum);
    sonicDestroyStream(stream);

    return 1;
}
