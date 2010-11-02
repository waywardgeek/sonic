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
This file supports read/write wave files.
*/
#include <stdlib.h>
#include <limits.h>
#include <sndfile.h>
#include "wave.h"

struct waveFileStruct {
    SNDFILE *soundFile;
    short *values;
    int numValues;
};

/* Open the file for reading.  Also determine it's sample rate. */
waveFile openInputWaveFile(
    char *fileName,
    int *sampleRate)
{
    SF_INFO info;
    SNDFILE *soundFile;
    waveFile file;

    info.format = 0;
    soundFile = sf_open(fileName, SFM_READ, &info);
    if(soundFile == NULL) {
	fprintf(stderr, "Unable to open wave file %s: %s\n", fileName, sf_strerror(NULL));
	return NULL;
    }
    file = (waveFile)calloc(1, sizeof(struct waveFileStruct));
    file->soundFile = soundFile;
    file->numValues = 42;
    file->values = (short *)calloc(file->numValues, sizeof(short));
    *sampleRate = info.samplerate;
    printf("Frames = %ld, sample rate = %d, channels = %d, format = %d",
        info.frames, info.samplerate, info.channels, info.format);
    return file;
}

/* Open the file for reading. */
waveFile openOutputWaveFile(
    char *fileName,
    int sampleRate)
{
    SF_INFO info;
    SNDFILE *soundFile;
    waveFile file;

    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    info.samplerate = sampleRate;
    info.channels = 1;
    soundFile = sf_open(fileName, SFM_WRITE, &info);
    if(soundFile == NULL) {
	fprintf(stderr, "Unable to open wave file %s: %s", fileName, sf_strerror(NULL));
	return NULL;
    }
    file = (waveFile)calloc(1, sizeof(struct waveFileStruct));
    file->soundFile = soundFile;
    file->numValues = 42;
    file->values = (short *)calloc(file->numValues, sizeof(short));
    return file;
}

/* Close the sound file. */
void closeWaveFile(
    waveFile file)
{
    SNDFILE *soundFile = file->soundFile;

    sf_close(soundFile);
}

/* Read from the wave file. */
int readFromWaveFile(
    waveFile file,
    float *buffer,
    int maxSamples)
{
    SNDFILE *soundFile = file->soundFile;
    int samplesRead;
    int i;

    if(maxSamples > file->numValues) {
	file->numValues = maxSamples;
	file->values = (short *)realloc(file->values, maxSamples*sizeof(short));
    }
    samplesRead = sf_read_short(soundFile, file->values, maxSamples);
    if(samplesRead <= 0) {
	return 0;
    }
    for(i = 0; i < samplesRead; i++) {
	buffer[i] = file->values[i]/32768.0;
    }
    return samplesRead;
}

/* Write to the wave file. */
int writeToWaveFile(
    waveFile file,
    float *buffer,
    int numSamples)
{
    SNDFILE *soundFile = file->soundFile;
    int value;
    int xValue, numWritten;

    if(numSamples > file->numValues) {
	file->numValues = numSamples*3/2;
	file->values = (short *)realloc(file->values, file->numValues*sizeof(short));
    }
    for(xValue = 0; xValue < numSamples; xValue++) {
	value = (int)(file->values[xValue]*32768.0 + 0.5);
	if(value > SHRT_MAX) {
	    file->values[xValue] = SHRT_MAX;
	} else if (value < SHRT_MIN) {
	    file->values[xValue] = SHRT_MIN;
	} else {
	    file->values[xValue] = value;
	}
    }
    numWritten = sf_write_short(soundFile, file->values, numSamples);
    if(numWritten != numSamples) {
	fprintf(stderr, "Unable to write wave file.\n");
	return 0;
    }
    return 1;
}
