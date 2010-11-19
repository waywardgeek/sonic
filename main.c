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

/*
Read wave files and speed them up or slow them down.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sonic.h"
#include "wave.h"

#define BUFFER_SIZE 2048

/* Scan through the input file to find it's maximum value. */
static int findMaximumVolume(
    waveFile inFile)
{
    short inBuffer[BUFFER_SIZE];
    int samplesRead, i;
    int maxValue = 0, value;

    do {
        samplesRead = readFromWaveFile(inFile, inBuffer, BUFFER_SIZE);
	for(i = 0; i < samplesRead; i++) {
	    value = inBuffer[i];
	    if(value < 0) {
		value = -value;
	    }
	    if(value > maxValue) {
		maxValue = value;
	    }
	}
    } while(samplesRead > 0);
    return maxValue;
}

/* Scale the samples by the factor. */
static void scaleSamples(
    short *samples,
    int numSamples,
    double scale)
{
    while(numSamples--) {
	*samples = (short)(*samples*scale + 0.5);
	samples++;
    }
}

/* Run sonic. */
static void runSonic(
    waveFile inFile,
    waveFile outFile,
    double speed,
    double scale,
    int sampleRate,
    int numChannels)
{
    sonicStream stream = sonicCreateStream(speed, sampleRate, numChannels);
    short inBuffer[BUFFER_SIZE], outBuffer[BUFFER_SIZE];
    int samplesRead, samplesWritten;

    do {
        samplesRead = readFromWaveFile(inFile, inBuffer, BUFFER_SIZE/numChannels);
	if(samplesRead == 0) {
	    sonicFlushStream(stream);
	} else {
	    if(scale != 1.0) {
		scaleSamples(inBuffer, samplesRead, scale);
	    }
	    sonicWriteShortToStream(stream, inBuffer, samplesRead);
	}
	do {
	    samplesWritten = sonicReadShortFromStream(stream, outBuffer,
	        BUFFER_SIZE/numChannels);
	    if(samplesWritten > 0) {
		writeToWaveFile(outFile, outBuffer, samplesWritten);
	    }
	} while(samplesWritten > 0);
    } while(samplesRead > 0);
    sonicDestroyStream(stream);
}

/* Print the usage. */
static void usage(void)
{
    fprintf(stderr, "Usage: sonic [-s speed] [-v volume] infile outfile\n"
        "    -s -- Set speed up factor.  1.0 means no change, 2.0 means 2X faster\n"
	"    -v -- Scale volume as percentage of maximum allowed.  100 uses full range.\n");
    exit(1);
}

int main(
    int argc,
    char **argv)
{
    waveFile inFile, outFile;
    char *inFileName, *outFileName;
    double speed = 1.0;
    int setVolume = 0;
    int maxVolume;
    double volume = 1.0;
    int sampleRate, numChannels;
    double scale = 1.0;
    int xArg = 1;

    while(xArg < argc && *(argv[xArg]) == '-') {
	if(!strcmp(argv[xArg], "-s")) {
	    xArg++;
	    if(xArg < argc) {
	        speed = atof(argv[xArg]);
                printf("Setting speed to %f\n", speed);
	    }
	} else if(!strcmp(argv[xArg], "-v")) {
	    xArg++;
	    if(xArg < argc) {
		setVolume = 1;
	        volume = atof(argv[xArg]);
                printf("Setting volume to %0.2f%%\n", volume);
		volume /= 100.0;
	    }
	}
	xArg++;
    }
    if(argc -xArg != 2) {
	usage();
    }
    inFileName = argv[xArg];
    outFileName = argv[xArg + 1];
    inFile = openInputWaveFile(inFileName, &sampleRate, &numChannels);
    if(inFile == NULL) {
	return 1;
    }
    outFile = openOutputWaveFile(outFileName, sampleRate, numChannels);
    if(outFile == NULL) {
	closeWaveFile(inFile);
	return 1;
    }
    if(setVolume) {
	maxVolume = findMaximumVolume(inFile);
	if(maxVolume != 0) {
	    scale = volume*32768.0f/maxVolume;
	}
	printf("Scale = %0.2f\n", scale);
	closeWaveFile(inFile);
	inFile = openInputWaveFile(inFileName, &sampleRate, &numChannels);
	if(inFile == NULL) {
	    closeWaveFile(outFile);
	    return 1;
	}
    }
    runSonic(inFile, outFile, speed, scale, sampleRate, numChannels);
    closeWaveFile(inFile);
    closeWaveFile(outFile);
    return 0;
}
