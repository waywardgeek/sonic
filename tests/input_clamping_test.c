/* Sonic library
   Copyright 2025
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

/* Unfortunate Google compatibility cruft. */
#ifdef GOOGLE_BUILD
#include "third_party/sonic/sonic.h"
#else
#include "sonic.h"
#endif

#include "genwave.h"
#include "tests.h"

#include <assert.h>

/* Just verify that we clamp to expected input values. */
int sonicTestInputClamping(void) {
  sonicStream stream = sonicCreateStream(44100, 1);
  sonicSetVolume(stream, SONIC_MIN_VOLUME * 0.9f);
  if (sonicGetVolume(stream) != SONIC_MIN_VOLUME) {
    return 0;
  }
  sonicSetVolume(stream, SONIC_MAX_VOLUME * 1.1f);
  if (sonicGetVolume(stream) != SONIC_MAX_VOLUME) {
    return 0;
  }
  sonicSetSpeed(stream, SONIC_MIN_SPEED * 0.9f);
  if (sonicGetSpeed(stream) != SONIC_MIN_SPEED) {
    return 0;
  }
  sonicSetSpeed(stream, SONIC_MAX_SPEED * 1.1f);
  if (sonicGetSpeed(stream) != SONIC_MAX_SPEED) {
    return 0;
  }
  sonicSetPitch(stream, SONIC_MIN_PITCH_SETTING * 0.9f);
  if (sonicGetPitch(stream) != SONIC_MIN_PITCH_SETTING) {
    return 0;
  }
  sonicSetPitch(stream, SONIC_MAX_PITCH_SETTING * 1.1f);
  if (sonicGetPitch(stream) != SONIC_MAX_PITCH_SETTING) {
    return 0;
  }
  sonicSetSpeed(stream, SONIC_MAX_SPEED * 1.1f);
  if (sonicGetSpeed(stream) != SONIC_MAX_SPEED) {
    return 0;
  }
  sonicSetRate(stream, SONIC_MIN_RATE * 0.9f);
  if (sonicGetRate(stream) != SONIC_MIN_RATE) {
    return 0;
  }
  sonicSetRate(stream, SONIC_MAX_RATE * 1.1f);
  if (sonicGetRate(stream) != SONIC_MAX_RATE) {
    return 0;
  }
  sonicSetSpeed(stream, SONIC_MAX_SPEED * 1.1f);
  if (sonicGetSpeed(stream) != SONIC_MAX_SPEED) {
    return 0;
  }
  sonicSetSampleRate(stream, (int)(SONIC_MIN_SAMPLE_RATE * 0.9f));
  if (sonicGetSampleRate(stream) != SONIC_MIN_SAMPLE_RATE) {
    return 0;
  }
  sonicSetSampleRate(stream, (int)(SONIC_MAX_SAMPLE_RATE * 1.1f));
  if (sonicGetSampleRate(stream) != SONIC_MAX_SAMPLE_RATE) {
    return 0;
  }
  sonicSetNumChannels(stream, 0);
  if (sonicGetNumChannels(stream) != SONIC_MIN_CHANNELS) {
    return 0;
  }
  sonicSetNumChannels(stream, SONIC_MAX_CHANNELS * 2);
  if (sonicGetNumChannels(stream) != SONIC_MAX_CHANNELS) {
    return 0;
  }
  sonicDestroyStream(stream);
  return 1;
}

/* Used as the read buffer length when processing audio. */
#define READ_BUF_LEN 1000

/* Process a few pitch periods of a sine wave. */
static void processSomeSamples(sonicStream stream, const short* samples, int numSamples) {
  short readBuf[READ_BUF_LEN];
  int samplesRead;

  /* Write all at once. */
  assert(sonicWriteShortToStream(stream, samples, numSamples));
  while ((samplesRead = sonicReadShortFromStream(stream, readBuf, READ_BUF_LEN)) != 0);
}

/* Constants defining a sine wave for tests. */
#define SAMPLE_RATE 44100
#define FREQ 200
#define PERIOD (SAMPLE_RATE / FREQ)
#define AMPLITUDE 6000
#define NUM_PERIODS 500
#define NUM_SAMPLES (NUM_PERIODS * PERIOD)

/* Test that the min and max values do not crash.  This does not test all
   combinations of min/max values. */
int sonicTestInputsDontCrash(void) {
  short samples[NUM_SAMPLES];
  int numSamples = genSineWave(samples, NUM_SAMPLES, SAMPLE_RATE, PERIOD,
                               AMPLITUDE, NUM_PERIODS);
  sonicStream stream;

  assert(numSamples == NUM_SAMPLES);
  stream = sonicCreateStream(SAMPLE_RATE, 1);
  sonicSetVolume(stream, SONIC_MIN_VOLUME);
  processSomeSamples(stream, samples, numSamples);
  sonicSetVolume(stream, SONIC_MAX_VOLUME);
  processSomeSamples(stream, samples, numSamples);
  sonicSetSpeed(stream, SONIC_MIN_SPEED);
  processSomeSamples(stream, samples, numSamples);
  sonicSetSpeed(stream, SONIC_MAX_SPEED);
  processSomeSamples(stream, samples, numSamples);
  sonicSetPitch(stream, SONIC_MIN_PITCH_SETTING);
  processSomeSamples(stream, samples, numSamples);
  sonicSetPitch(stream, SONIC_MAX_PITCH_SETTING);
  processSomeSamples(stream, samples, numSamples);
  sonicSetSpeed(stream, SONIC_MAX_SPEED);
  processSomeSamples(stream, samples, numSamples);
  sonicSetRate(stream, SONIC_MIN_RATE);
  processSomeSamples(stream, samples, numSamples);
  sonicSetRate(stream, SONIC_MAX_RATE);
  processSomeSamples(stream, samples, numSamples);
  sonicSetSpeed(stream, SONIC_MAX_SPEED);
  processSomeSamples(stream, samples, numSamples);
  sonicSetSampleRate(stream, SONIC_MIN_SAMPLE_RATE);
  processSomeSamples(stream, samples, numSamples);
  sonicSetSampleRate(stream, SONIC_MAX_SAMPLE_RATE);
  processSomeSamples(stream, samples, numSamples);
  sonicSetNumChannels(stream, SONIC_MIN_CHANNELS);
  processSomeSamples(stream, samples, numSamples / SONIC_MIN_CHANNELS);
  sonicSetNumChannels(stream, SONIC_MAX_CHANNELS);
  processSomeSamples(stream, samples, numSamples / SONIC_MAX_CHANNELS);
  sonicDestroyStream(stream);
  return 1;
}
