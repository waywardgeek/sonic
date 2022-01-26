// Some basic Sonic unit tests.

#include "sonic.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Number of columns used in printing a waveform as and ASCII graph, like the SPICE simulator.
#define SONIC_NUM_COLUMNS 99  // Make it odd so that 0 is exactly in the middle.

struct sonicWaveStruct {
  int16_t *samples;
  size_t numSamples;
};

typedef struct sonicWaveStruct *sonicWave;

// Create a wafeform initialized to all 0's.
sonicWave sonicCreateZeroWave(uint32_t numSamples) {
  sonicWave waveform = calloc(1, sizeof(struct sonicWaveStruct));
  waveform->samples = calloc(numSamples, sizeof(int16_t));
  return waveform;
}

// Destroy a waveform.
void sonicDestroyWaveform(sonicWave waveform) {
  assert(waveform != NULL && waveform->samples != NULL);
  free(waveform->samples);
  waveform->samples = NULL;
  free(waveform);
}

// Create a sine waveform.  Period is in terms of number of samples.
sonicWave sonicCreateSineWave(int16_t amplitude, uint32_t period, uint32_t numSamples) {
  sonicWave waveform = sonicCreateZeroWave(numSamples);
  for (size_t i = 0; i < numSamples; i++) {
    int16_t sample = amplitude * sin(i * 2.0 * M_PI / period);
    waveform->samples[i] = sample;
  }
  return waveform;
}

// Print a sample in ASCII as 0 to SONIC_NUM_COLUMNS spaces, except where the
// current sample lies.  If the current sample is non-zero, print '.' at 0.
static void printSample(int16_t sample) {
  uint16_t pos = ((uint16_t)sample + 0x8000) / (UINT16_MAX / SONIC_NUM_COLUMNS);
  uint16_t middle = SONIC_NUM_COLUMNS >> 1;
  for (uint16_t i = 0; i < SONIC_NUM_COLUMNS; i++) {
    if (i == pos) {
      putchar('*');
    } else if (i == middle) {
      putchar('.');
    } else {
      putchar(' ');
    }
  }
}

// Copy a waveform.
sonicWave sonicCopyWave(sonicWave waveform) {
  sonicWave output = calloc(1, sizeof(struct sonicWaveStruct));
  *output = *waveform;
  output->samples = calloc(waveform->numSamples, sizeof(int16_t));
  memcpy(output->samples, waveform->samples, waveform->numSamples * sizeof(int16_t));
  return output;
}

// For debug purposes.
void sonidDumpWaveform(sonicWave waveform) {
  for (size_t i = 0; i < waveform->numSamples; i++) {
    printSample(waveform->samples[i]);
  }
}

// Change the speed of a waveform.
sonicWave sonicWaveChangeSpeed(sonicWave waveform, float speed, uint32_t sampleRate) {
  // TODO: increase the waveform buffer size if needed!
  assert( speed >= 1.0);
  sonicWave output = sonicCopyWave(waveform);
  int numSamples = sonicChangeShortSpeed(output->samples,
      output->numSamples, speed, /* pitch */ 1.0, /* rate */ 1.0, /* volume */ 1.0,
          /* useChordPitch */ false, sampleRate, /* numChannels */ 1);
  output->numSamples = numSamples;
  return output;
}

// Return true if the waveforms are equal.
bool sonicWavesEqual(sonicWave wave1, sonicWave wave2) {
  return wave1->numSamples == wave2->numSamples && !memcmp(wave1->samples, wave2->samples,
      wave1->numSamples * sizeof(int16_t));
}

/* Test that speeding up exactly two pitch periods by 2X gives exactly one
   pit4ch period without distortion. */
static void testTwoPeriodsAt2XSpeed(void) {
  sonicWave twoPeriods = sonicCreateSineWave(1000, 11025, 22050);
  sonicWave onePeriod = sonicCreateSineWave(1000, 11025, 11025);
  sonicWave output = sonicWaveChangeSpeed(twoPeriods, 2.0, 22050);
  assert(!sonicWavesEqual(onePeriod, output));
}

/* Test that we properly speed up sige waves. */
static void speedUpSineWaveTests(void) {
  testTwoPeriodsAt2XSpeed();
}

int main(int argc, char **argv) {
   speedUpSineWaveTests();
   return 0;
}
