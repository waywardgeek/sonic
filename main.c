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

/* Run sonic. */
static void runSonic(
    waveFile inFile,
    waveFile outFile,
    float speed,
    float pitch,
    float volume,
    int quality,
    int sampleRate,
    int numChannels)
{
    sonicStream stream = sonicCreateStream(sampleRate, numChannels);
    short inBuffer[BUFFER_SIZE], outBuffer[BUFFER_SIZE];
    int samplesRead, samplesWritten;

    sonicSetSpeed(stream, speed);
    sonicSetPitch(stream, pitch);
    sonicSetVolume(stream, volume);
    sonicSetQuality(stream, quality);
    do {
        samplesRead = readFromWaveFile(inFile, inBuffer, BUFFER_SIZE/numChannels);
	if(samplesRead == 0) {
	    sonicFlushStream(stream);
	} else {
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
    fprintf(stderr, "Usage: sonic [OPTION]... infile outfile\n"
        "    -p pitch   -- Set pitch scaling factor.  1.3 means 30%% higher.\n"
        "    -q         -- Disable speed-up heuristics.  May increase quality.\n"
        "    -s speed   -- Set speed up factor.  2.0 means 2X faster.\n"
	"    -v volume  -- Scale volume by a constant factor.\n");
    exit(1);
}

int main(
    int argc,
    char **argv)
{
    waveFile inFile, outFile;
    char *inFileName, *outFileName;
    float speed = 1.0;
    float pitch = 1.0;
    float volume = 1.0;
    int quality = 0;
    int sampleRate, numChannels;
    int xArg = 1;

    if(argc < 2 || *(argv[xArg]) != '-') {
	fprintf(stderr, "You must provide at least one option to change speed,"
	    "pitch, or volume.\n");
	usage();
	return 1;
    }
    while(xArg < argc && *(argv[xArg]) == '-') {
	if(!strcmp(argv[xArg], "-p")) {
	    xArg++;
	    if(xArg < argc) {
	        pitch = atof(argv[xArg]);
                printf("Setting pitch to %0.2f%%\n", pitch*100.0f);
	    }
	} else if(!strcmp(argv[xArg], "-q")) {
	    xArg++;
	    quality = 1;
	    printf("Disabling speed-up heuristics\n");
	} else if(!strcmp(argv[xArg], "-s")) {
	    xArg++;
	    if(xArg < argc) {
	        speed = atof(argv[xArg]);
                printf("Setting speed to %0.2f%%\n", speed*100.0f);
	    }
	} else if(!strcmp(argv[xArg], "-v")) {
	    xArg++;
	    if(xArg < argc) {
	        volume = atof(argv[xArg]);
                printf("Setting volume to %0.2f\n", volume);
	    }
	}
	xArg++;
    }
    if(argc - xArg != 2) {
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
    runSonic(inFile, outFile, speed, pitch, volume, quality, sampleRate, numChannels);
    closeWaveFile(inFile);
    closeWaveFile(outFile);
    return 0;
}
