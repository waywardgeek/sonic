/* Sonic library
   Copyright 2010
   Bill Cox
   This file is part of the Sonic Library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sonic.h"

struct sonicStreamStruct {
    double speed;
    float *inputBuffer;
    float *outputBuffer;
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
    double speed,
    int sampleRate)
{
    sonicStream stream = (sonicStream)calloc(1, sizeof(struct sonicStreamStruct));
    int minPeriod = sampleRate/SONIC_MAX_PITCH;
    int maxPeriod = sampleRate/SONIC_MIN_PITCH;
    int maxRequired = 2*maxPeriod; 

    if(stream == NULL) {
	return NULL;
    }
    stream->inputBuffer = (float *)calloc(maxRequired, sizeof(float));
    if(stream->inputBuffer == NULL) {
	sonicDestroyStream(stream);
	return NULL;
    }
    stream->outputBuffer = (float *)calloc(maxRequired, sizeof(float));
    if(stream->outputBuffer == NULL) {
	sonicDestroyStream(stream);
	return NULL;
    }
    stream->speed = speed;
    stream->sampleRate = sampleRate;
    stream->minPeriod = minPeriod;
    stream->maxPeriod = maxPeriod;
    stream->maxRequired = maxRequired;
    stream->inputBufferSize = maxRequired;
    stream->outputBufferSize = maxRequired;
    return stream;
}

/* Find the pitch period.  This is a critical step, and we may have to try
   multiple ways to get a good answer.  This version uses AMDF. */
static int findPitchPeriod(
    sonicStream stream,
    float *samples)
{
    int minPeriod = stream->minPeriod;
    int maxPeriod = stream->maxPeriod;
    int period, bestPeriod = 0;
    double minDiff = 0.0;
    double diff, value;
    int xSample;

    for(period = minPeriod; period <= maxPeriod; period++) {
	diff = 0.0;
	for(xSample = 0; xSample < period; xSample++) {
	    value = samples[xSample] - samples[xSample + period];
	    diff += value >= 0.0? value : -value;
	}
	diff /= period;
	if(bestPeriod == 0 || diff < minDiff) {
	    minDiff = diff;
	    bestPeriod = period;
	}
    }
    return bestPeriod;
}

/* Enlarge the output buffer if needed. */
static int enlargeOutputBufferIfNeeded(
    sonicStream stream,
    int numSamples)
{
    if(stream->numOutputSamples + numSamples > stream->outputBufferSize) {
	stream->outputBufferSize += (stream->outputBufferSize >> 1) + numSamples;
	stream->outputBuffer = (float *)realloc(stream->outputBuffer,
	    stream->outputBufferSize*sizeof(float));
	if(stream->outputBuffer == NULL) {
            sonicDestroyStream(stream);
	    return 0;
	}
    }
    return 1;
}

/* Do the pitch based resampling for one pitch period of the input. */
static int resamplePitchPeriod(
    sonicStream stream,
    float *samples,
    double speed,
    int period)
{
    int t, newSamples;
    double scale;
    float *out;

    if(speed >= 2.0) {
	newSamples = period/(speed - 1.0);
    } else if(speed > 1.0) {
	newSamples = period;
	stream->remainingInputToCopy = period*(2.0 - speed)/(speed - 1.0);
    } else {
	fprintf(stderr, "Speed currently must be > 1\n");
	exit(1);
    }
    scale = 1.0/newSamples;
    if(!enlargeOutputBufferIfNeeded(stream, newSamples)) {
	return 0;
    }
    out = stream->outputBuffer + stream->numOutputSamples;
    for(t = 0; t < newSamples; t++) {
        out[t] = scale*(samples[t]*(newSamples - t) + samples[t + period]*t);
    }
    stream->numOutputSamples += newSamples;
    return newSamples;
}

/* Enlarge the input buffer if needed. */
static int enlargeInputBufferIfNeeded(
    sonicStream stream,
    int numSamples)
{
    if(stream->numInputSamples + numSamples > stream->inputBufferSize) {
	stream->inputBufferSize += (stream->inputBufferSize >> 1) + numSamples;
	stream->inputBuffer = (float *)realloc(stream->inputBuffer,
	    stream->inputBufferSize*sizeof(float));
	if(stream->inputBuffer == NULL) {
            sonicDestroyStream(stream);
	    return 0;
	}
    }
    return 1;
}

/* Add the input samples to the input buffer. */
static int addSamplesToInputBuffer(
    sonicStream stream,
    float *samples,
    int numSamples)
{
    if(numSamples == 0) {
	return 1;
    }
    if(!enlargeInputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    memcpy(stream->inputBuffer + stream->numInputSamples, samples, numSamples*sizeof(float));
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
	memmove(stream->inputBuffer, stream->inputBuffer + position,
	    remainingSamples*sizeof(float));
    }
    stream->numInputSamples = remainingSamples;
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
    if(!enlargeOutputBufferIfNeeded(stream, numSamples)) {
	return 0;
    }
    memcpy(stream->outputBuffer + stream->numOutputSamples, stream->inputBuffer + position,
        numSamples*sizeof(float));
    stream->numOutputSamples += numSamples;
    stream->remainingInputToCopy -= numSamples;
    return numSamples;
}

/* Resample as many pitch periods as we have buffered on the input.  Return 0 if
   we fail to resize an input or output buffer */
int sonicWriteToStream(
    sonicStream stream,
    float *samples,
    int numSamples)
{
    double speed = stream->speed;
    int position = 0, period, newSamples;
    int maxRequired = stream->maxRequired;

    if(!addSamplesToInputBuffer(stream, samples, numSamples)) {
	return 0;
    }
    if(stream->numInputSamples < maxRequired) {
	return 1;
    }
    samples = stream->inputBuffer;
    numSamples = stream->numInputSamples;
    do {
	if(stream->remainingInputToCopy > 0) {
	    period = 0;
            newSamples = copyInputToOutput(stream, position);
	} else {
	    period = findPitchPeriod(stream, samples + position);
	    newSamples = resamplePitchPeriod(stream, samples + position, speed, period);
	}
	if(newSamples == 0) {
	    return 0; /* Failed to resize output buffer */
	}
        position += period + newSamples;
    } while(position + maxRequired <= numSamples);
    removeInputSamples(stream, position);
    return 1;
}

/* Read data out of the stream.  Sometimes no data will be available, and zero
   is returned, which is not an error condition. */
int sonicReadFromStream(
    sonicStream stream,
    float *samples,
    int maxSamples)
{
    int numSamples = stream->numOutputSamples;
    int remainingSamples = 0;

    if(numSamples == 0) {
	return 0;
    }
    if(numSamples > maxSamples) {
	numSamples = maxSamples;
	remainingSamples = numSamples - maxSamples;
    }
    memcpy(samples, stream->outputBuffer, numSamples*sizeof(float));
    if(remainingSamples > 0) {
	memmove(stream->outputBuffer, stream->outputBuffer + numSamples,
	    remainingSamples*sizeof(float));
    }
    stream->numOutputSamples = remainingSamples;
    return numSamples;
}

/* Force the sonic stream to generate output using whatever data it currently
   has.  Zeros will be appended to the input data if there is not enough data
   in the stream's input buffer.  Use this, followed by a final read from the
   stream before destroying the stream. */
int sonicFlushStream(
    sonicStream stream)
{
    int maxRequired = stream->maxRequired;
    int numSamples = stream->numInputSamples;
    int remainingSpace;

    if(numSamples == 0) {
	return 1;
    }
    if(numSamples >= maxRequired && !sonicWriteToStream(stream, NULL, 0)) {
	return 0;
    }
    numSamples = stream->numInputSamples; /* Now numSamples < maxRequired */
    remainingSpace = maxRequired - numSamples;
    memset(stream->inputBuffer + numSamples, 0, remainingSpace*sizeof(float));
    stream->numInputSamples = maxRequired;
    return sonicWriteToStream(stream, NULL, 0);
}

/* Return the number of samples in the output buffer */
int sonicSamplesAvailale(
   sonicStream stream)
{
    return stream->numOutputSamples;
}
