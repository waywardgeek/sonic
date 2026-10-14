#include <math.h>
#include <stdio.h>

#include "wave.h"
#include "genwave.h"

#include <assert.h>
#include <stdlib.h>

#ifndef M_PI
# define M_PI		3.1415926535897932384
#endif

/* Write samples to the outFile. */
static void writeSamplesToFile(char* fileName, int sampleRate, short* samples, int numSamples) {
  waveFile outFile = openOutputWaveFile(fileName, sampleRate, 1);
  int written;

  if (outFile == NULL) {
    fprintf(stderr, "Unable to open %s\n", fileName);
    abort();
  }
  written = writeToWaveFile(outFile, samples, numSamples);
  if (!written) {
    fprintf(stderr, "Unable to write %s\n", fileName);
    abort();
  }
  closeWaveFile(outFile);
}

int main(int argc, char** argv) {
  int sampleRate = 96000;
  int freq = 200;
  int period = sampleRate / freq;
  int amplitude = 6000;
  int numPeriods = 500;
  int numSamples = period * numPeriods;
  short* samples = malloc(numSamples * sizeof(short));

  numSamples = genSineWave(samples, numSamples, sampleRate, period, amplitude, numPeriods);
  writeSamplesToFile("out.wav", sampleRate, samples, numSamples);
  free(samples);
  return 0;
}
