/* Sonic library
   Copyright 2026
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#include "wave.h"

#include <stdio.h>

#define TEST_FILE_NAME "wave_channel_validation_test.wav"
/* Byte offset of the numChannels field within the wave header written by
   wave.c's writeHeader: 4 (RIFF) + 4 (size) + 4 (WAVE) + 4 (fmt ) + 4
   (chunk size) + 2 (format) = 22. */
#define NUM_CHANNELS_OFFSET 22

/* Verify a wave file whose header claims 0 channels is rejected by
   openInputWaveFile instead of being accepted with an unusable channel
   count -- callers (e.g. the sonic CLI) divide by numChannels without
   checking it, so letting 0 through causes a division by zero. Write a
   normal, valid file via the public API, then corrupt just the
   numChannels field on disk to simulate a malformed/adversarial input
   file, since wave.h doesn't expose header construction directly. */
int sonicTestWaveRejectsZeroChannels(void) {
    waveFile file;
    FILE* rawFile;
    short zero = 0;
    int sampleRate, numChannels;

    file = openOutputWaveFile(TEST_FILE_NAME, 44100, 1);
    if (file == NULL) {
        fprintf(stderr, "openOutputWaveFile failed\n");
        return 0;
    }
    closeWaveFile(file);

    rawFile = fopen(TEST_FILE_NAME, "r+b");
    if (rawFile == NULL) {
        fprintf(stderr, "Unable to reopen %s for corruption\n", TEST_FILE_NAME);
        return 0;
    }
    fseek(rawFile, NUM_CHANNELS_OFFSET, SEEK_SET);
    fwrite(&zero, sizeof(short), 1, rawFile);
    fclose(rawFile);

    file = openInputWaveFile(TEST_FILE_NAME, &sampleRate, &numChannels);
    remove(TEST_FILE_NAME);
    if (file != NULL) {
        fprintf(stderr,
                "openInputWaveFile should reject a header with 0 channels\n");
        closeWaveFile(file);
        return 0;
    }
    return 1;
}
