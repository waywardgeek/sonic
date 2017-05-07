/* This file was written by Bill Cox in 2010, and is licensed under the Apache
   2.0 license.

   This file is meant as a simple example for how to use libsonic.  It is also a
   useful utility on its own, which can speed up or slow down wav files, change
   pitch, and scale volume. */

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
    float rate,
    float volume,
    int emulateChordPitch,
    int quality,
    int enableNonlinearSpeedup,
    int computeSpectrogram,
    int numRows,
    int numCols,
    int sampleRate,
    int numChannels)
{
    sonicStream stream = sonicCreateStream(sampleRate, numChannels);
    short inBuffer[BUFFER_SIZE], outBuffer[BUFFER_SIZE];
    int samplesRead, samplesWritten;

    sonicSetSpeed(stream, speed);
    sonicSetPitch(stream, pitch);
    sonicSetRate(stream, rate);
    sonicSetVolume(stream, volume);
    sonicSetChordPitch(stream, emulateChordPitch);
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
        "    -c         -- Modify pitch by emulating vocal chords vibrating\n"
        "                  faster or slower.\n"
        "    -n         -- Enable nonlinear speedup\n"
        "    -p pitch   -- Set pitch scaling factor.  1.3 means 30%% higher.\n"
        "    -q         -- Disable speed-up heuristics.  May increase quality.\n"
        "    -r rate    -- Set playback rate.  2.0 means 2X faster, and 2X pitch.\n"
        "    -s speed   -- Set speed up factor.  2.0 means 2X faster.\n"
        "    -S width height -- Generate a spectrogram in sonic.pgm in PGM format.\n"
        "    -v volume  -- Scale volume by a constant factor.\n");
    exit(1);
}

#include <limits.h>
double findSincCoefficient(int N, double x);

int main(
    int argc,
    char **argv)
{
    waveFile inFile, outFile;
    char *inFileName, *outFileName;
    float speed = 1.0f;
    float pitch = 1.0f;
    float rate = 1.0f;
    float volume = 1.0f;
    int emulateChordPitch = 0;
    int quality = 0;
    int sampleRate, numChannels;
    int xArg = 1;
    int enableNonlinearSpeedup = 0;
    int computeSpectrogram = 0;
    int numRows = 0, numCols = 0;

    while(xArg < argc && *(argv[xArg]) == '-') {
        if(!strcmp(argv[xArg], "-c")) {
            emulateChordPitch = 1;
            printf("Scaling pitch linearly.\n");
        } else if(!strcmp(argv[xArg], "-n")) {
            enableNonlinearSpeedup = 1;
            printf("Enabling nonlinear speedup.\n");
        } else if(!strcmp(argv[xArg], "-p")) {
            xArg++;
            if(xArg < argc) {
                pitch = atof(argv[xArg]);
                printf("Setting pitch to %0.2fX\n", pitch);
            }
        } else if(!strcmp(argv[xArg], "-q")) {
            quality = 1;
            printf("Disabling speed-up heuristics\n");
        } else if(!strcmp(argv[xArg], "-r")) {
            xArg++;
            if(xArg < argc) {
                rate = atof(argv[xArg]);
                if (rate == 0.0f) {
                    usage();
                }
                printf("Setting rate to %0.2fX\n", rate);
            }
        } else if(!strcmp(argv[xArg], "-s")) {
            xArg++;
            if(xArg < argc) {
                speed = atof(argv[xArg]);
                printf("Setting speed to %0.2fX\n", speed);
            }
        } else if(!strcmp(argv[xArg], "-S")) {
            xArg++;
            if(xArg < argc) {
                numCols = atof(argv[xArg]);
            }
            xArg++;
            if(xArg < argc) {
                numRows = atof(argv[xArg]);
                computeSpectrogram = 1;
                printf("Computing spectrogram %d wide and %d tall\n", numCols, numRows);
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
    runSonic(inFile, outFile, speed, pitch, rate, volume, emulateChordPitch, quality,
        enableNonlinearSpeedup, computeSpectrogram, numRows, numCols, sampleRate, numChannels);
    closeWaveFile(inFile);
    closeWaveFile(outFile);
    return 0;
}
