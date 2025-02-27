#include <math.h>
#include <stdio.h>

/* Unfortunate Google compatibility cruft. */
#ifdef GOOGLE_BUILD
#include "third_party/sonic/wave.h"
#else
#include "wave.h"
#endif

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

/* Write a sine wave to an output buffer.  Return the number of samples written.
 */
int genSineWave(short* output, int outputLen, int sampleRate, int period,
                int amplitude, int numPeriods) {
  int i, j;
  short value;
  double x;
  int numSamples = 0;

  for (i = 0; i < numPeriods; i++) {
    for (j = 0; j < period; j++) {
      if (numSamples == outputLen) {
        return numSamples;
      }
      x = (double)j * (2.0 * M_PI) / period;
      value = (short)(amplitude * sin(x));
      output[numSamples++] = value;
    }
  }
  return numSamples;
}
