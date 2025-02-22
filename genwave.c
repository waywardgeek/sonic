#include <math.h>
#include <stdio.h>

#include "wave.h"

#ifndef M_PI
# define M_PI		3.1415926535897932384
#endif

/* Write a sine wave to outFile. */
static void genSineWave(waveFile outFile, int sampleRate, int period, int amplitude, int numPeriods) {
  int i, j;
  short value;
  double x;

  for (i = 0; i < numPeriods; i++) {
    for (j = 0; j < period; j++) {
      x = (double)j * (2.0 * M_PI) / period;
      value = (short)(amplitude * sin(x));
      writeToWaveFile(outFile, &value, 1);
    }
  }
}

int main(int argc, char** argv) {
  int sampleRate = 96000;
  int freq = 200;
  int period = sampleRate / freq;
  int amplitude = 6000;
  waveFile outFile = openOutputWaveFile("out.wav", sampleRate, 1);

  genSineWave(outFile, sampleRate, period, amplitude, 500);
  closeWaveFile(outFile);
  return 0;
}
