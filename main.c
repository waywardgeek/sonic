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
    waveFile inFile,
    waveFile outFile,
    double speed,
    int sampleRate)
{
    sonicStream stream = sonicCreateStream(speed, sampleRate);
    short inBuffer[BUFFER_SIZE], outBuffer[BUFFER_SIZE];
    int samplesRead, samplesWritten;

    do {
        samplesRead = readFromWaveFile(inFile, inBuffer, BUFFER_SIZE);
	if(samplesRead == 0) {
	    sonicFlushStream(stream);
	} else {
	    sonicWriteShortToStream(stream, inBuffer, samplesRead);
	}
	do {
	    samplesWritten = sonicReadShortFromStream(stream, outBuffer, BUFFER_SIZE);
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
    fprintf(stderr, "Usage: sonic speed infile outfile\n");
    exit(1);
}

int main(
    int argc,
    char **argv)
{
    waveFile inFile, outFile;
    char *inFileName, *outFileName;
    double speed;
    int sampleRate;

    if(argc != 4) {
	usage();
    }
    speed = atof(argv[1]);
    inFileName = argv[2];
    outFileName = argv[3];
    inFile = openInputWaveFile(inFileName, &sampleRate);
    if(inFile == NULL) {
	return 1;
    }
    outFile = openOutputWaveFile(outFileName, sampleRate);
    if(outFile == NULL) {
	closeWaveFile(inFile);
	return 1;
    }
    runSonic(inFile, outFile, speed, sampleRate);
    closeWaveFile(inFile);
    closeWaveFile(outFile);
    return 0;
}
