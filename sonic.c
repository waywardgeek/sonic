/* Sonic library
   Copyright 2010
   Bill Cox
   This file is part of the Sonic Library.

   The Sonic Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sonic.h"

struct sonicStreamStruct {
    short *inputBuffer;
    short *outputBuffer;
    float speed;
    int numChannels;
    int inputBufferSize;
    int outputBufferSize;
    int numInputSamples;
    int numOutputSamples;
    int minPeriod;
    int maxPeriod;
    int maxRequired;
    int remainingInputToCopy;
    int sampleRate;
};

/* Just used for debugging */
void sonicMSG(char *format, ...)
{
    char buffer[4096];
    va_list ap;
    FILE *file;

    va_start(ap, format);
    vsprintf((char *)buffer, (char *)format, ap);
    va_end(ap);
    file=fopen("/tmp/sonic.log", "a");
    fprintf(file, "%s", buffer);
    fclose(file);
}

/* Get the speed of the stream. */
float sonicGetSpeed(
    sonicStream stream)
{
    return stream->speed;
}

/* Get the sample rate of the stream. */
int sonicGetSampleRate(
    sonicStream stream)
{
    return stream->sampleRate;
}

/* Destroy the sonic stream. */
void sonicDestroyStream(
    sonicStream stream)
{
    if(stream->inputBuffer != NULL) {
	free(stream->inputBuffer);
    }
    if(stream->outputBuffer != NULL) {
	free(stream->outputBuffer);
    }
    free(stream);
}

/* Create a sonic stream.  Return NULL only if we are out of memory and cannot
   allocate the stream. */
sonicStream sonicCreateStream(
    float speed,
    int sampleRate,
    int numChannels)
{
    sonicStream stream = (sonicStream)calloc(1, sizeof(struct sonicStreamStruct));
    int minPeriod = sampleRate/SONIC_MAX_PITCH;
    int maxPeriod = sampleRate/SONIC_MIN_PITCH;
    int maxRequired = 2*maxPeriod; 

    if(stream == NULL) {
	return NULL;
    }
    stream->inputBuffer = (short *)calloc(maxRequired, sizeof(short)*numChannels);
    if(stream->inputBuffer == NULL) {
	sonicDestroyStream(stream);
	return NULL;
    }
    stream->outputBuffer = (short *)calloc(maxRequired, sizeof(short)*numChannels);
    if(stream->outputBuffer == NULL) {
	sonicDestroyStream(stream);
	return NULL;
    }
    stream->speed = speed;
    stream->sampleRate = sampleRate;
    stream->numChannels = numChannels;
    stream->minPeriod = minPeriod;
    stream->maxPeriod = maxPeriod;
    stream->maxRequired = maxRequired;
    stream->inputBufferSize = maxRequired;
    stream->outputBufferSize = maxRequired;
    return stream;
}

/* Enlarge the output buffer if needed. */
static int enlargeOutputBufferIfNeeded(
    sonicStream stream,
    int numSamples)
{
    if(stream->numOutputSamples + numSamples > stream->outputBufferSize) {
	stream->outputBufferSize += (stream->outputBufferSize >> 1) + numSamples;
	stream->outputBuffer = (short *)realloc(stream->outputBuffer,
	    stream->outputBufferSize*sizeof(short)*stream->numChannels);
	if(stream->outputBuffer == NULL) {
	    return 0;
	}
    }
    return 1;
}

/* Enlarge the input buffer if needed. */
static int enlargeInputBufferIfNeeded(
    sonicStream stream,
    int numSamples)
{
    if(stream->numInputSamples + numSamples > stream->inputBufferSize) {
	stream->inputBufferSize += (stream->inputBufferSize >> 1) + numSamples;
	stream->inputBuffer = (short *)realloc(stream->inputBuffer,
	    stream->inputBufferSize*sizeof(short)*stream->numChannels);
	if(stream->inputBuffer == NULL) {
	    return 0;
	}
    }
    return 1;
}

/* Add the input samples to the input buffer. */
static int addFloatSamplesToInputBuffer(
    sonicStream stream,
    float *samples,
    int numSamples)
{
    short *buffer;
    int count = numSamples*stream->numChannels;

    if(numSamples == 0) {
	return 1;
    }
    if(!enlargeInputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    buffer = stream->inputBuffer + stream->numInputSamples*stream->numChannels;
    while(count--) {
        *buffer++ = (*samples++)*32767.0f;
    }
    stream->numInputSamples += numSamples;
    return 1;
}

/* Add the input samples to the input buffer. */
static int addShortSamplesToInputBuffer(
    sonicStream stream,
    short *samples,
    int numSamples)
{
    if(numSamples == 0) {
	return 1;
    }
    if(!enlargeInputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    memcpy(stream->inputBuffer + stream->numInputSamples*stream->numChannels, samples,
        numSamples*sizeof(short)*stream->numChannels);
    stream->numInputSamples += numSamples;
    return 1;
}

/* Add the input samples to the input buffer. */
static int addUnsignedCharSamplesToInputBuffer(
    sonicStream stream,
    unsigned char *samples,
    int numSamples)
{
    short *buffer;
    int count = numSamples*stream->numChannels;

    if(numSamples == 0) {
	return 1;
    }
    if(!enlargeInputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    buffer = stream->inputBuffer + stream->numInputSamples*stream->numChannels;
    while(count--) {
        *buffer++ = (*samples++ - 128) << 8;
    }
    stream->numInputSamples += numSamples;
    return 1;
}

/* Remove input samples that we have already processed. */
static void removeInputSamples(
    sonicStream stream,
    int position)
{
    int remainingSamples = stream->numInputSamples - position;

    if(remainingSamples > 0) {
	memmove(stream->inputBuffer, stream->inputBuffer + position*stream->numChannels,
	    remainingSamples*sizeof(short)*stream->numChannels);
    }
    stream->numInputSamples = remainingSamples;
}

/* Just copy from the array to the output buffer */
static int copyFloatToOutput(
    sonicStream stream,
    float *samples,
    int numSamples)
{
    short *buffer;
    int count = numSamples*stream->numChannels;

    if(!enlargeOutputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    buffer = stream->outputBuffer + stream->numOutputSamples*stream->numChannels;
    while(count--) {
        *buffer++ = (*samples++)*32767.0f;
    }
    stream->numOutputSamples += numSamples;
    return numSamples;
}

/* Just copy from the array to the output buffer */
static int copyShortToOutput(
    sonicStream stream,
    short *samples,
    int numSamples)
{
    if(!enlargeOutputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    memcpy(stream->outputBuffer + stream->numOutputSamples*stream->numChannels,
	samples, numSamples*sizeof(short)*stream->numChannels);
    stream->numOutputSamples += numSamples;
    return numSamples;
}

/* Just copy from the array to the output buffer */
static int copyUnsignedCharToOutput(
    sonicStream stream,
    unsigned char *samples,
    int numSamples)
{
    short *buffer;
    int count = numSamples*stream->numChannels;

    if(!enlargeOutputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    buffer = stream->outputBuffer + stream->numOutputSamples*stream->numChannels;
    while(count--) {
        *buffer++ = (*samples++ - 128) << 8;
    }
    stream->numOutputSamples += numSamples;
    return numSamples;
}

/* Just copy from the input buffer to the output buffer.  Return 0 if we fail to
   resize the output buffer.  Otherwise, return numSamples */
static int copyInputToOutput(
    sonicStream stream,
    int position)
{
    int numSamples = stream->remainingInputToCopy;

    if(numSamples > stream->maxRequired) {
	numSamples = stream->maxRequired;
    }
    if(!copyShortToOutput(stream, stream->inputBuffer + position*stream->numChannels,
	    numSamples)) {
	return 0;
    }
    stream->remainingInputToCopy -= numSamples;
    return numSamples;
}

/* Read data out of the stream.  Sometimes no data will be available, and zero
   is returned, which is not an error condition. */
int sonicReadFloatFromStream(
    sonicStream stream,
    float *samples,
    int maxSamples)
{
    int numSamples = stream->numOutputSamples;
    int remainingSamples = 0;
    short *buffer;
    int count;

    if(numSamples == 0) {
	return 0;
    }
    if(numSamples > maxSamples) {
	remainingSamples = numSamples - maxSamples;
	numSamples = maxSamples;
    }
    buffer = stream->outputBuffer;
    count = numSamples*stream->numChannels;
    while(count--) {
	*samples++ = (*buffer++)/32767.0f;
    }
    if(remainingSamples > 0) {
	memmove(stream->outputBuffer, stream->outputBuffer + numSamples*stream->numChannels,
	    remainingSamples*sizeof(short)*stream->numChannels);
    }
    stream->numOutputSamples = remainingSamples;
    return numSamples;
}

/* Read short data out of the stream.  Sometimes no data will be available, and zero
   is returned, which is not an error condition. */
int sonicReadShortFromStream(
    sonicStream stream,
    short *samples,
    int maxSamples)
{
    int numSamples = stream->numOutputSamples;
    int remainingSamples = 0;

    if(numSamples == 0) {
	return 0;
    }
    if(numSamples > maxSamples) {
	remainingSamples = numSamples - maxSamples;
	numSamples = maxSamples;
    }
    memcpy(samples, stream->outputBuffer, numSamples*sizeof(short)*stream->numChannels);
    if(remainingSamples > 0) {
	memmove(stream->outputBuffer, stream->outputBuffer + numSamples*stream->numChannels,
	    remainingSamples*sizeof(short)*stream->numChannels);
    }
    stream->numOutputSamples = remainingSamples;
    return numSamples;
}

/* Read unsigned char data out of the stream.  Sometimes no data will be available, and zero
   is returned, which is not an error condition. */
int sonicReadUnsignedCharFromStream(
    sonicStream stream,
    unsigned char *samples,
    int maxSamples)
{
    int numSamples = stream->numOutputSamples;
    int remainingSamples = 0;
    short *buffer;
    int count;

    if(numSamples == 0) {
	return 0;
    }
    if(numSamples > maxSamples) {
	remainingSamples = numSamples - maxSamples;
	numSamples = maxSamples;
    }
    buffer = stream->outputBuffer;
    count = numSamples*stream->numChannels;
    while(count--) {
	*samples++ = (char)((*buffer++) >> 8) + 128;
    }
    if(remainingSamples > 0) {
	memmove(stream->outputBuffer, stream->outputBuffer + numSamples*stream->numChannels,
	    remainingSamples*sizeof(short)*stream->numChannels);
    }
    stream->numOutputSamples = remainingSamples;
    return numSamples;
}

/* Force the sonic stream to generate output using whatever data it currently
   has.  No extra delay will be added to the output, but flushing in the middle of
   words could introduce distortion. */
int sonicFlushStream(
    sonicStream stream)
{
    int maxRequired = stream->maxRequired;
    int numSamples = stream->numInputSamples;
    int remainingSpace, numOutputSamples, expectedSamples;

    if(numSamples == 0) {
	return 1;
    }
    if(numSamples >= maxRequired && !sonicWriteShortToStream(stream, NULL, 0)) {
	return 0;
    }
    numSamples = stream->numInputSamples; /* Now numSamples < maxRequired */
    if(numSamples == 0) {
	return 1;
    }
    remainingSpace = maxRequired - numSamples;
    memset(stream->inputBuffer + numSamples*stream->numChannels, 0,
        remainingSpace*sizeof(short)*stream->numChannels);
    stream->numInputSamples = maxRequired;
    numOutputSamples = stream->numOutputSamples;
    if(!sonicWriteShortToStream(stream, NULL, 0)) {
	return 0;
    }
    /* Throw away any extra samples we generated due to the silence we added */
    expectedSamples = (int)(numSamples*stream->speed + 0.5);
    if(stream->numOutputSamples > numOutputSamples + expectedSamples) {
	stream->numOutputSamples = numOutputSamples + expectedSamples;
    }
    return 1;
}

/* Return the number of samples in the output buffer */
int sonicSamplesAvailable(
   sonicStream stream)
{
    return stream->numOutputSamples;
}

/* Find the best frequency match in the range, and given a sample skip multiple.
   For now, just find the pitch of the first channel.  */
static int findPitchPeriodInRange(
    short *samples,
    int numChannels,
    int minPeriod,
    int maxPeriod,
    int skip)
{
    int period, bestPeriod = 0;
    short *s, *p, sVal, pVal;
    unsigned long diff, minDiff = 0;
    unsigned int numSamples = (minPeriod + skip - 1)/skip;
    unsigned int bestNumSamples = 0;
    int sampleSkip = skip*numChannels;
    int i;

    for(period = minPeriod; period <= maxPeriod; period += skip) {
	diff = 0;
	s = samples;
	p = samples + period*numChannels;
	for(i = 0; i < numSamples; i++) {
	    sVal = *s;
	    pVal = *p;
	    s += sampleSkip;
	    p += sampleSkip;
	    diff += sVal >= pVal? (unsigned short)(sVal - pVal) :
	        (unsigned short)(pVal - sVal);
	}
	/* Note that the highest number of samples we add into diff will be less
	   than 256, since we skip samples.  Thus, diff is a 24 bit number, and
	   we can safely multiply by numSamples without overflow */
	if(bestPeriod == 0 || diff*bestNumSamples < minDiff*numSamples) {
	    minDiff = diff;
	    bestPeriod = period;
	    bestNumSamples = numSamples;
	}
	numSamples++;
    }
    return bestPeriod;
}

/* Find the pitch period.  This is a critical step, and we may have to try
   multiple ways to get a good answer.  This version uses AMDF.  To improve
   speed, we down sample by an integer factor get in the 11KHz range, and then
   do it again with a narrower frequency range without down sampling */
static int findPitchPeriod(
    sonicStream stream,
    short *samples)
{
    int minPeriod = stream->minPeriod;
    int maxPeriod = stream->maxPeriod;
    int sampleRate = stream->sampleRate;
    int skip = 1;
    int period;

    if(sampleRate > SONIC_AMDF_FREQ) {
	skip = sampleRate/SONIC_AMDF_FREQ;
    }
    period = findPitchPeriodInRange(samples, stream->numChannels, minPeriod, maxPeriod, skip);
    if(skip == 1) {
	return period;
    }
    minPeriod = period - (skip << 2);
    maxPeriod = period + (skip << 2);
    if(minPeriod < stream->minPeriod) {
	minPeriod = stream->minPeriod;
    }
    if(maxPeriod > stream->maxPeriod) {
	maxPeriod = stream->maxPeriod;
    }
    return findPitchPeriodInRange(samples, stream->numChannels, minPeriod, maxPeriod, 1);
}

/* Skip over a pitch period, and copy period/speed samples to the output */
static int skipPitchPeriod(
    sonicStream stream,
    short *samples,
    float speed,
    int period)
{
    long t, newSamples;
    short *out, *prevPeriodSamples, *nextPeriodSamples;
    int numChannels = stream->numChannels;
    int i;

    if(speed >= 2.0f) {
	newSamples = period/(speed - 1.0f);
    } else if(speed > 1.0f) {
	newSamples = period;
	stream->remainingInputToCopy = period*(2.0f - speed)/(speed - 1.0f);
    }
    if(!enlargeOutputBufferIfNeeded(stream, newSamples)) {
	return 0;
    }
    for(i = 0; i < numChannels; i++) {
	out = stream->outputBuffer + stream->numOutputSamples*numChannels + i;
	prevPeriodSamples = samples + i;
	nextPeriodSamples = samples + i + period*numChannels;
	for(t = 0; t < newSamples; t++) {
	    *out = (*prevPeriodSamples*(newSamples - t) + *nextPeriodSamples*t)/newSamples;
	    out += numChannels;
	    prevPeriodSamples += numChannels;
	    nextPeriodSamples += numChannels;
	}
    }
    stream->numOutputSamples += newSamples;
    return newSamples;
}

/* Insert a pitch period, and determine how much input to copy directly. */
static int insertPitchPeriod(
    sonicStream stream,
    short *samples,
    float speed,
    int period)
{
    long t, newSamples;
    short *out, *prevPeriodSamples, *nextPeriodSamples;
    int numChannels = stream->numChannels;
    int i;

    if(speed < 0.5f) {
        newSamples = period*speed/(1.0f - speed);
    } else {
        newSamples = period;
	stream->remainingInputToCopy = period*(2.0f*speed - 1.0f)/(1.0f - speed);
    }
    if(!enlargeOutputBufferIfNeeded(stream, period + newSamples)) {
	return 0;
    }
    out = stream->outputBuffer + stream->numOutputSamples*numChannels;
    memcpy(out, samples, period*sizeof(short)*numChannels);
    for(i = 0; i < numChannels; i++) {
	out = stream->outputBuffer + (stream->numOutputSamples + period)*numChannels + i;
	prevPeriodSamples = samples + i;
	nextPeriodSamples = samples + i + period*numChannels;
	for(t = 0; t < newSamples; t++) {
	    *out = (*prevPeriodSamples*t + *nextPeriodSamples*(newSamples - t))/newSamples;
	    out += numChannels;
	    prevPeriodSamples += numChannels;
	    nextPeriodSamples += numChannels;
	}
    }
    stream->numOutputSamples += period + newSamples;
    return newSamples;
}

/* Resample as many pitch periods as we have buffered on the input.  Return 0 if
   we fail to resize an input or output buffer */
static int processStreamInput(
    sonicStream stream)
{
    short *samples;
    int numSamples = stream->numInputSamples;
    float speed = stream->speed;
    int position = 0, period, newSamples;
    int maxRequired = stream->maxRequired;

    if(stream->numInputSamples < maxRequired) {
	return 1;
    }
    do {
	if(stream->remainingInputToCopy > 0) {
            newSamples = copyInputToOutput(stream, position);
	    position += newSamples;
	} else {
	    samples = stream->inputBuffer + position*stream->numChannels;
	    period = findPitchPeriod(stream, samples);
	    if(speed > 1.0) {
		newSamples = skipPitchPeriod(stream, samples, speed, period);
		position += period + newSamples;
	    } else {
		newSamples = insertPitchPeriod(stream, samples, speed, period);
		position += newSamples;
	    }
	}
	if(newSamples == 0) {
	    return 0; /* Failed to resize output buffer */
	}
    } while(position + maxRequired <= numSamples);
    removeInputSamples(stream, position);
    return 1;
}

/* Write floating point data to the input buffer and process it. */
int sonicWriteFloatToStream(
    sonicStream stream,
    float *samples,
    int numSamples)
{
    float speed = stream->speed;

    if(speed > 0.999999 && speed < 1.000001) {
	/* No speed change - just copy to the output */
	return copyFloatToOutput(stream, samples, numSamples);
    }
    if(!addFloatSamplesToInputBuffer(stream, samples, numSamples)) {
	return 0;
    }
    return processStreamInput(stream);
}

/* Simple wrapper around sonicWriteFloatToStream that does the short to float
   conversion for you. */
int sonicWriteShortToStream(
    sonicStream stream,
    short *samples,
    int numSamples)
{
    float speed = stream->speed;

    if(speed > 0.999999 && speed < 1.000001) {
	/* No speed change - just copy to the output */
	return copyShortToOutput(stream, samples, numSamples);
    }
    if(!addShortSamplesToInputBuffer(stream, samples, numSamples)) {
	return 0;
    }
    return processStreamInput(stream);
}

/* Simple wrapper around sonicWriteFloatToStream that does the unsigned char to float
   conversion for you. */
int sonicWriteUnsignedCharToStream(
    sonicStream stream,
    unsigned char *samples,
    int numSamples)
{
    float speed = stream->speed;

    if(speed > 0.999999 && speed < 1.000001) {
	/* No speed change - just copy to the output */
	return copyUnsignedCharToOutput(stream, samples, numSamples);
    }
    if(!addUnsignedCharSamplesToInputBuffer(stream, samples, numSamples)) {
	return 0;
    }
    return processStreamInput(stream);
}

/* This is a non-stream oriented interface to just change the speed of a sound sample */
int sonicChangeFloatSpeed(
    float *samples,
    int numSamples,
    float speed,
    int sampleRate,
    int numChannels)
{
    sonicStream stream = sonicCreateStream(speed, sampleRate, numChannels);

    sonicWriteFloatToStream(stream, samples, numSamples);
    sonicFlushStream(stream);
    numSamples = sonicSamplesAvailable(stream);
    sonicReadFloatFromStream(stream, samples, numSamples);
    sonicDestroyStream(stream);
    return numSamples;
}

/* This is a non-stream oriented interface to just change the speed of a sound sample */
int sonicChangeShortSpeed(
    short *samples,
    int numSamples,
    float speed,
    int sampleRate,
    int numChannels)
{
    sonicStream stream = sonicCreateStream(speed, sampleRate, numChannels);

    sonicWriteShortToStream(stream, samples, numSamples);
    sonicFlushStream(stream);
    numSamples = sonicSamplesAvailable(stream);
    sonicReadShortFromStream(stream, samples, numSamples);
    sonicDestroyStream(stream);
    return numSamples;
}
