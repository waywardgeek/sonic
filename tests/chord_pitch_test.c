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

#define CONSTANT_LEVEL 1000

/* Samples at the end of a flushed stream blend toward the silence flush pads
   with, so only the stream's body is held to the input level. */
#define FLUSH_TAIL 2000

/* Feed numSamples of a constant signal through a chord-pitch stream and read
   everything back.  Returns the number of samples read, or -1 if any sample
   before the flush tail differs from the input level: a constant input must
   come out constant, whatever the pitch period, because the crossfade
   weights sum to one. */
static int runConstantSignal(float pitch, float rate, int numSamples) {
    static short samples[NUM_SAMPLES];
    short readBuf[READ_BUF_LEN];
    sonicStream stream = sonicCreateStream(SAMPLE_RATE, 1);
    int total = 0;
    int firstBad = -1;
    int samplesRead, i;

    for (i = 0; i < numSamples; i++) {
        samples[i] = CONSTANT_LEVEL;
    }
    sonicSetChordPitch(stream, 1);
    sonicSetPitch(stream, pitch);
    sonicSetRate(stream, rate);
    sonicWriteShortToStream(stream, samples, numSamples);
    sonicFlushStream(stream);
    while ((samplesRead = sonicReadShortFromStream(stream, readBuf,
                                                    READ_BUF_LEN)) != 0) {
        for (i = 0; i < samplesRead; i++) {
            if (readBuf[i] != CONSTANT_LEVEL && firstBad < 0) {
                firstBad = total + i;
            }
        }
        total += samplesRead;
    }
    sonicDestroyStream(stream);
    if (firstBad >= 0 && firstBad < total - FLUSH_TAIL) {
        return -1;
    }
    return total;
}

/* A pitch below 1 repeats each input period.  The repeats must be crossfaded
   with weights summing to one, and a gap (pitch <= 0.5) must be filled by
   repeating the period, so a constant input never dips or goes negative. */
int sonicTestChordPitchPreservesConstantSignal(void) {
    static const float pitches[] = {0.05f, 0.3f, 0.5f, 0.75f, 1.0f, 1.5f, 4.0f};
    int i;

    for (i = 0; i < (int)(sizeof(pitches) / sizeof(pitches[0])); i++) {
        if (runConstantSignal(pitches[i], 1.0f, NUM_SAMPLES) < 0) {
            fprintf(stderr, "constant signal changed at chord pitch %f\n",
                    pitches[i]);
            return 0;
        }
    }
    return 1;
}

/* sonicSetRate must change the output length in chord-pitch mode too:
   rate 2 plays the stream twice as fast. */
int sonicTestChordPitchAppliesRate(void) {
    int normal = runConstantSignal(1.0f, 1.0f, NUM_SAMPLES);
    int doubled = runConstantSignal(1.0f, 2.0f, NUM_SAMPLES);

    if (normal < 0 || doubled < 0) {
        fprintf(stderr, "constant signal changed with chord pitch rate\n");
        return 0;
    }
    if (doubled < normal * 45 / 100 || doubled > normal * 55 / 100) {
        fprintf(stderr, "rate 2 gave %d samples, expected about half of %d\n",
                doubled, normal);
        return 0;
    }
    return 1;
}

/* A NaN pitch must not reach the period / pitch float-to-int conversion. */
int sonicTestSetPitchRejectsNaN(void) {
    volatile float zero = 0.0f;
    float nan = zero / zero;
    sonicStream stream = sonicCreateStream(SAMPLE_RATE, 1);
    float pitch;

    sonicSetPitch(stream, nan);
    pitch = sonicGetPitch(stream);
    sonicDestroyStream(stream);
    return pitch >= SONIC_MIN_PITCH_SETTING && pitch <= SONIC_MAX_PITCH_SETTING;
}

/* Samples buffered for pitch adjustment must still be emitted after pitch
   moves back to 1.0 mid-stream.  Dropping them lets flush's silence padding
   reach the stream's last samples, so with a constant input every output
   sample, tail included, must keep the input level. */
int sonicTestChordPitchReturnToUnityDrainsBuffer(void) {
    static short samples[NUM_SAMPLES];
    short readBuf[READ_BUF_LEN];
    sonicStream stream = sonicCreateStream(SAMPLE_RATE, 1);
    int half = NUM_SAMPLES / 2;
    int samplesRead, i;

    for (i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = CONSTANT_LEVEL;
    }
    sonicSetChordPitch(stream, 1);
    sonicSetPitch(stream, 1.3f);
    sonicWriteShortToStream(stream, samples, half);
    sonicSetPitch(stream, 1.0f);
    sonicFlushStream(stream);
    while ((samplesRead = sonicReadShortFromStream(stream, readBuf,
                                                    READ_BUF_LEN)) != 0) {
        for (i = 0; i < samplesRead; i++) {
            if (readBuf[i] != CONSTANT_LEVEL) {
                fprintf(stderr, "sample %d is %d, expected %d\n", i,
                        readBuf[i], CONSTANT_LEVEL);
                sonicDestroyStream(stream);
                return 0;
            }
        }
    }
    sonicDestroyStream(stream);
    return 1;
}
