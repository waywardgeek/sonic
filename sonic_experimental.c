/* Sonic library
   Copyright 2010
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

/* This file is designed for low-powered microcontrollers, minimizing memory
   compared to the fuller sonic.c implementation. */

#include "sonic_experimental.h"

/* temp */
#include <stdio.h>
#include <assert.h>

#include <string.h>

#define SONIC_INPUT_BUFFER_SIZE (3 * (SONIC_MAX_SAMPLE_RATE / SONIC_MIN_PITCH) + SONIC_INPUT_SAMPLES)
static int sonicMinPeriod, sonicMaxPeriod;

struct sonicStruct {
  /* The input buffer will have at least 3 pitch periods.  The sample at
     sonicMaxPeriod is the first new unprocessed sample.  We keep the prior
     samples up to sonicMaxPeriod so we can find the snippet at this point for
     any pitch period.  This is used when transitioning from the current
     snippet to the next. */
  short inputBuffer[1000000];
  short outputBuffer[1000000];
  short downSampleBuffer[1000000];
  float speed;
  int sampleRate;
  int numInputSamples;
  int snippetPeriod; 
  int snippetOffset; 
  int numOutputSamples;
  int prevPeriod;
  int prevMinDiff;
};

static struct sonicStruct sonicStream;

/* A snippet is computed around an input in the input buffer by first applying
   a 2-period Hann window, and then overlap-adding the left half to the right.
   It should essentially sound like the input at that point in time. */
struct sonicSnippetStruct {
  short samples[100000];
  int inputPos;  /* Index into input buffer. */
  int offset;
  int period;
};

typedef struct sonicSnippetStruct* sonicSnippet;

/* Set the speed of the stream. */
void sonicSetSpeed(float speed) { sonicStream.speed = speed; }

/* Set the sample rate of the stream. */
void sonicSetSampleRate(int sampleRate) {
}

/* Create a sonic stream.  Return NULL only if we are out of memory and cannot
   allocate the stream. */
void sonicInit(float speed, int sampleRate) {
  sonicStream.speed = speed;
  sonicStream.sampleRate = sampleRate;
  sonicMinPeriod = sampleRate / SONIC_MAX_PITCH;
  sonicMaxPeriod = sampleRate / SONIC_MIN_PITCH;
  memset(&sonicStream, 0, sizeof(struct sonicStruct));
  sonicStream.speed = speed;
  sonicStream.sampleRate = sampleRate;
  sonicStream.numInputSamples = 0;
  sonicStream.numOutputSamples = 0;
  sonicStream.prevPeriod = 0;
  sonicStream.prevMinDiff = 0;
  sonicStream.numInputSamples = sonicMinPeriod;
  sonicStream.snippetPeriod = sonicMinPeriod;
  sonicStream.snippetOffset = 0;
}

/* Add the input samples to the input buffer. */
static int addShortSamplesToInputBuffer(short *samples,
                                        int numSamples) {
  if (numSamples == 0) {
    return 1;
  }
  if (sonicStream.numInputSamples + numSamples > SONIC_INPUT_BUFFER_SIZE) {
    return 0;
  }
/* temp */
{
int i;
for (i = 1; i < numSamples; i++) {
  printf("%d %d\n", samples[i], samples[i] - samples[i-1]);
}
}
  memcpy(sonicStream.inputBuffer + sonicStream.numInputSamples,
         samples, numSamples * sizeof(short));
  sonicStream.numInputSamples += numSamples;
  return 1;
}

/* Remove input samples that we have already processed. */
static void removeInputSamples(int position) {
  int remainingSamples = sonicStream.numInputSamples - position;
/* temp */
assert(position >= sonicMaxPeriod);

  if (remainingSamples > 0) {
    memmove(sonicStream.inputBuffer,
            sonicStream.inputBuffer + position - sonicMaxPeriod,
            (remainingSamples + sonicMaxPeriod) * sizeof(short));
  }
  sonicStream.numInputSamples = sonicMaxPeriod + remainingSamples;
}

/* Read short data out of the stream.  Sometimes no data will be available, and
   zero is returned, which is not an error condition. */
int sonicReadShortFromStream(short *samples, int maxSamples) {
  int numSamples = sonicStream.numOutputSamples;
  int remainingSamples = 0;

  if (numSamples == 0) {
    return 0;
  }
  if (numSamples > maxSamples) {
    remainingSamples = numSamples - maxSamples;
    numSamples = maxSamples;
  }
  memcpy(samples, sonicStream.outputBuffer, numSamples * sizeof(short));
  if (remainingSamples > 0) {
    memmove(sonicStream.outputBuffer, sonicStream.outputBuffer + numSamples,
            remainingSamples * sizeof(short));
  }
  sonicStream.numOutputSamples = remainingSamples;
  return numSamples;
}

/* Force the sonic stream to generate output using whatever data it currently
   has.  No extra delay will be added to the output, but flushing in the middle
   of words could introduce distortion. */
void sonicFlushStream(void) {
  int remainingSamples = sonicStream.numInputSamples - sonicMaxPeriod;
  float speed = sonicStream.speed;
  int expectedOutputSamples = sonicStream.numOutputSamples + (int)((remainingSamples / speed) + 0.5f);

  memset(sonicStream.inputBuffer + sonicMaxPeriod + remainingSamples, 0,
      sizeof(short) * (SONIC_INPUT_BUFFER_SIZE - (sonicMaxPeriod + remainingSamples)));
  sonicStream.numInputSamples = SONIC_INPUT_BUFFER_SIZE;
  sonicWriteShortToStream(NULL, 0);
  /* Throw away any extra samples we generated due to the silence we added */
  if (sonicStream.numOutputSamples > expectedOutputSamples) {
    sonicStream.numOutputSamples = expectedOutputSamples;
  }
  /* Empty input buffer */
  sonicStream.numInputSamples = sonicMinPeriod;
  memset(sonicStream.inputBuffer, 0, SONIC_INPUT_BUFFER_SIZE * sizeof(short));
}

/* Return the number of samples in the output buffer */
int sonicSamplesAvailable(void) {
  return sonicStream.numOutputSamples;
}

/* If skip is greater than one, average skip samples together and write them to
   the down-sample buffer. */
static void downSampleInput(short *samples) {
  int numSamples = 2 * sonicMaxPeriod;
  int i, j;
  int value;
  short *downSamples = sonicStream.downSampleBuffer;
  int skip = sonicStream.sampleRate / SONIC_AMDF_FREQ;

  for (i = 0; i < numSamples; i++) {
    value = 0;
    for (j = 0; j < skip; j++) {
      value += *samples++;
    }
    value /= skip;
    *downSamples++ = value;
  }
}

/* Find the best frequency match in the range, and given a sample skip multiple.
   For now, just find the pitch of the first channel. */
static int findPitchPeriodInRange(short *samples, int minPeriod, int maxPeriod,
                                  int* retMinDiff, int* retMaxDiff) {
  int period, bestPeriod = 0, worstPeriod = 255;
  short *s;
  short *p;
  short sVal, pVal;
  unsigned long diff, minDiff = 1, maxDiff = 0;
  int i;

  for (period = minPeriod; period <= maxPeriod; period++) {
    diff = 0;
    s = samples;
    p = samples + period;
    for (i = 0; i < period; i++) {
      sVal = *s++;
      pVal = *p++;
      diff += sVal >= pVal ? (unsigned short)(sVal - pVal)
                           : (unsigned short)(pVal - sVal);
    }
    /* Note that the highest number of samples we add into diff will be less
       than 256, since we skip samples.  Thus, diff is a 24 bit number, and
       we can safely multiply by numSamples without overflow */
    if (bestPeriod == 0 || diff * bestPeriod < minDiff * period) {
      minDiff = diff;
      bestPeriod = period;
    }
    if (diff * worstPeriod > maxDiff * period) {
      maxDiff = diff;
      worstPeriod = period;
    }
  }
  *retMinDiff = minDiff / bestPeriod;
  *retMaxDiff = maxDiff / worstPeriod;
  return bestPeriod;
}

/* At abrupt ends of voiced words, we can have pitch periods that are better
   approximated by the previous pitch period estimate.  Try to detect this case.  */
static int prevPeriodBetter(int minDiff, int maxDiff, int preferNewPeriod) {
  if (minDiff == 0 || sonicStream.prevPeriod == 0) {
    return 0;
  }
  if (preferNewPeriod) {
    if (maxDiff > minDiff * 3) {
      /* Got a reasonable match this period */
      return 0;
    }
    if (minDiff * 2 <= sonicStream.prevMinDiff * 3) {
      /* Mismatch is not that much greater this period */
      return 0;
    }
  } else {
    if (minDiff <= sonicStream.prevMinDiff) {
      return 0;
    }
  }
  return 1;
}

/* Find the pitch period.  This is a critical step, and we may have to try
   multiple ways to get a good answer.  This version uses Average Magnitude
   Difference Function (AMDF).  To improve speed, we down sample by an integer
   factor get in the 11KHz range, and then do it again with a narrower
   frequency range without down sampling */
static int findPitchPeriod(short *samples, int preferNewPeriod) {
  int minPeriod = sonicMinPeriod;
  int maxPeriod = sonicMaxPeriod;
  int minDiff, maxDiff, retPeriod;
  int period;
  int skip = sonicStream.sampleRate / SONIC_AMDF_FREQ;

  if (skip == 1) {
    period = findPitchPeriodInRange(samples, minPeriod, maxPeriod, &minDiff, &maxDiff);
  } else {
    downSampleInput(samples);
    period = findPitchPeriodInRange(sonicStream.downSampleBuffer, minPeriod / skip,
                                    maxPeriod / skip, &minDiff, &maxDiff);
    period *= skip;
    minPeriod = period - (skip << 2);
    maxPeriod = period + (skip << 2);
    if (minPeriod < sonicMinPeriod) {
      minPeriod = sonicMinPeriod;
    }
    if (maxPeriod > sonicMaxPeriod) {
      maxPeriod = sonicMaxPeriod;
    }
    period = findPitchPeriodInRange(samples, minPeriod, maxPeriod, &minDiff, &maxDiff);
  }
  if (prevPeriodBetter(minDiff, maxDiff, preferNewPeriod)) {
    retPeriod = sonicStream.prevPeriod;
  } else {
    retPeriod = period;
  }
  sonicStream.prevMinDiff = minDiff;
  sonicStream.prevPeriod = period;
  return retPeriod;
}

/* Overlap two sound segments, ramp the volume of one down, while ramping the
   other one from zero up, and add them, storing the result at the output. */
static void overlapAdd(int numSamples, short *out, short *rampDown, short *rampUp) {
  short *o;
  short *u;
  short *d;
  int t;

  o = out;
  u = rampUp;
  d = rampDown;
  for (t = 0; t < numSamples; t++) {
    *o = (*d * (numSamples - t) + *u * t) / numSamples;
    o++;
    d++;
    u++;
  }
}

/* temp: comput on the fly */
#include <math.h>
#ifndef M_PI
# define M_PI		3.1415926535897932384
#endif

/* Compute the sound snippet from the current input, and the prior input if
   needed. */
static void setPeriod(sonicSnippet snippet, int period) {
  int pos = snippet->inputPos;
  short* p = sonicStream.inputBuffer + pos;
  float fade;
  int i;
/* temp */
assert(p + period - 1 < sonicStream.inputBuffer + sonicStream.numInputSamples);
assert(p - period >= sonicStream.inputBuffer);

  snippet->period = period;
  for (i = 0; i < period; i++) {
    /* TODO: Make this a Hann window. */
/* temp */
/*    fade = (float)i / period; */
fade = 0.5*(1.0 - cos(M_PI*i/period));
    snippet->samples[i] = (1.0f - fade) * p[i] + fade * p[i - period];
  }
}

/* Write the output sample. */
static void outputSample(short value) {
/* temp */
static short oldValue = 0;
int diff = value - oldValue;
diff = diff < 0? -diff : diff;
oldValue = value;
printf("diff = %d\n", diff);

  sonicStream.outputBuffer[sonicStream.numOutputSamples++] = value;
}

/* Increment the offset into the snippent.  Set to 0 if we reach the period. */
static void incOffset(sonicSnippet snippet) {
  snippet->offset++;
  if (snippet->offset == snippet->period) {
    snippet->offset = 0;
  }
}

/* Fade from snippet A to snippet B smoothly. */
static void fadeFromAToB(sonicSnippet A, sonicSnippet B) {
  int numOutputSamples = B->period / sonicStream.speed;
  /* Initially snippet A and snippet B have different periods. */
  int periodB = B->period;
  int changedPeriod = 0;
  int i;
  float fadeA, fadeB;

  /* A’s offset may be non-zero from playing it in the prior iteration. */
  if (numOutputSamples <= A->period - A->offset) {
    /* We will fade out A before finishing it.  Just use B’s period
       for B.  Don’t use it for A as that would cause a discontinuity. */
    changedPeriod = 1;
  } else {
    /* Play B using A’s period until we reset the offset to 0. */
    setPeriod(B, A->period);
/* temp maybe will sound better using B's period? */
    B->offset = A->offset;
  }
/* temp */
assert(A->offset < A->period);
assert(B->offset < B->period);
  for (i = 0; i < numOutputSamples; i++) {
    if (!changedPeriod && A->offset == 0) {
/* temp */
assert(B->offset == 0);
      setPeriod(A, periodB);
      setPeriod(B, periodB);
      changedPeriod = 1;
    }
/* temp */
assert(A->offset < A->period);
assert(B->offset < B->period);
assert(A->offset >= 0 && B->offset >= 0);
    fadeB = (float) i / numOutputSamples;
    fadeA = 1.0 - fadeB;
    outputSample(fadeA * A->samples[A->offset] +
           fadeB * B->samples[B->offset]);
    incOffset(A);  /* Sets offset to 0 if offset == period. */
    incOffset(B);
  }
}

/* Set the offset of B to be in phase with A's offset. */
static void setBOffset(sonicSnippet A, sonicSnippet B) {
  int offset = A->offset;

  /* When pitch is increasing, offset can be > B->period. */
  while (offset >= B->period) {
    offset -= B->period;
  }
  B->offset = offset;
}

/* Determine if two snippets are identical other than for inputPos. */
static int snippetsEqual(sonicSnippet A, sonicSnippet B) {
  int i;

  if (A->period != B->period || A->offset != B->offset) {
    return 0;
  }
  for (i = 0; i < A->period; i++) {
    if (A->samples[i] != B->samples[i]) {
      return 0;
    }
  }
  return 1;
}

/* Process as many pitch periods as we have buffered on the input. */
static void changeSpeed(float speed) {
/* temp */
static struct sonicSnippetStruct A, B;
/*  struct sonicSnippetStruct A, B; */
  int period;
/* temp */
static int firstTime = 1;

  if (sonicStream.numInputSamples < 3 * sonicMaxPeriod) {
    return;
  }
  while (sonicStream.numInputSamples >= 3 * sonicMaxPeriod) {
    /* TODO: Don't recompute the snippet for A. */
    A.inputPos = sonicMaxPeriod;
    A.offset = sonicStream.snippetOffset;
    setPeriod(&A, sonicStream.snippetPeriod);
/* temp */
if (!firstTime) {
  assert(snippetsEqual(&A, &B));
}
firstTime = 0;
/* temp */
assert(A.offset < A.period);
    period = findPitchPeriod(sonicStream.inputBuffer + sonicMaxPeriod, 1);
    B.inputPos = sonicMaxPeriod + period;
    setPeriod(&B, period);
    setBOffset(&A, &B);
/* temp */
assert(B.offset < B.period);
    fadeFromAToB(&A, &B);
/* temp */
assert(A.offset < A.period);
assert(B.offset < B.period);
assert(B.period == period);
    removeInputSamples(B.inputPos);
    sonicStream.snippetPeriod = B.period;
    sonicStream.snippetOffset = B.offset;
  }
}

/* Just copy from the array to the output buffer */
static void copyToOutput(short *samples, int numSamples) {
  memcpy(sonicStream.outputBuffer + sonicStream.numOutputSamples,
         samples, numSamples * sizeof(short));
  sonicStream.numOutputSamples += numSamples;
}

/* Resample as many pitch periods as we have buffered on the input.  Also scale
   the output by the volume. */
static void processStreamInput(void) {
  float speed = sonicStream.speed;

  if (speed > 1.00001 || speed < 0.99999) {
    changeSpeed(speed);
  } else {
    copyToOutput(sonicStream.inputBuffer + sonicMaxPeriod,
        sonicStream.numInputSamples - sonicMaxPeriod);
    sonicStream.numInputSamples = sonicMaxPeriod;
  }
}

/* Simple wrapper around sonicWriteFloatToStream that does the short to float
   conversion for you. */
void sonicWriteShortToStream(short *samples, int numSamples) {
  addShortSamplesToInputBuffer(samples, numSamples);
  processStreamInput();
}

/* This is ignored. */
void sonicSetVolume(float volume) {
}
