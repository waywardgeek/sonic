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

/*
Read wave files and speed them up or slow them down.
*/

#include <stdio.h>
#include <stdlib.h>
#include "sonic.h"
#include "wave.h"

#define BUFFER_SIZE 1024

/* Run sonic. */
static void runSonic(
    char *inFileName,
    char *outFileName,
    double speed)
{
    int sampleRate;
    waveFile inFile = openInputWaveFile(inFileName, &sampleRate);
    waveFile outFile = openOutputWaveFile(outFileName, sampleRate);
    sonicStream stream = sonicCreateStream(speed, sampleRate);
    float inBuffer[BUFFER_SIZE], outBuffer[BUFFER_SIZE];
    int numSamples;

    while(1) {
        numSamples = readFromWaveFile(inFile, inBuffer, BUFFER_SIZE);
	if(numSamples == 0) {
	    sonicFlushStream(stream);
            sonicDestroyStream(stream);
	    closeWaveFile(inFile);
	    closeWaveFile(outFile);
	    return;
	}
        sonicWriteToStream(stream, inBuffer, numSamples);
	do {
	    numSamples = sonicReadFromStream(stream, outBuffer, BUFFER_SIZE);
	    if(numSamples > 0) {
		writeToWaveFile(outFile, outBuffer, numSamples);
	    }
	} while(numSamples > 0);
    }
}

/* Print the usage. */
static void usage(void)
{
    fprintf(stderr, "Usage: sonic speed infile outfile\n");
    exit(1);
}

int main(
    int argc,
    char **argv)
{
    char *inFileName, *outFileName;
    double speed;

    if(argc != 4) {
	usage();
    }
    speed = atof(argv[1]);
    inFileName = argv[2];
    outFileName = argv[3];
    runSonic(inFileName, outFileName, speed);
    return 0;
}
