/* Sonic library
   Copyright 2025
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#include "sonic.h"
#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2

int sonicTestStreamCreation(void) {
    sonicStream stream = sonicCreateStream(SAMPLE_RATE, NUM_CHANNELS);
    if (stream == NULL) {
        fprintf(stderr, "sonicCreateStream failed\n");
        return 0;
    }
    if (sonicGetSampleRate(stream) != SAMPLE_RATE) {
        fprintf(stderr, "sonicGetSampleRate failed\n");
        return 0;
    }
    if (sonicGetNumChannels(stream) != NUM_CHANNELS) {
        fprintf(stderr, "sonicGetNumChannels failed\n");
        return 0;
    }
    sonicDestroyStream(stream);
    return 1;
}

int sonicTestParameters(void) {
    sonicStream stream = sonicCreateStream(SAMPLE_RATE, NUM_CHANNELS);
    
    sonicSetSpeed(stream, 2.0f);
    if (sonicGetSpeed(stream) != 2.0f) return 0;
    
    sonicSetPitch(stream, 0.5f);
    if (sonicGetPitch(stream) != 0.5f) return 0;
    
    sonicSetRate(stream, 1.5f);
    if (sonicGetRate(stream) != 1.5f) return 0;
    
    sonicSetVolume(stream, 0.8f);
    if (sonicGetVolume(stream) != 0.8f) return 0;
    
    sonicSetQuality(stream, 1);
    if (sonicGetQuality(stream) != 1) return 0;
    
    sonicDestroyStream(stream);
    return 1;
}

int sonicTestFlush(void) {
    sonicStream stream = sonicCreateStream(SAMPLE_RATE, 1);
    short samples[100] = {0};
    int ok;

    if (stream == NULL) {
        return 0;
    }
    ok = sonicWriteShortToStream(stream, samples, 100);
    sonicFlushStream(stream);
    /* At unit speed, flush must leave exactly the 100 written samples. */
    ok = ok && sonicSamplesAvailable(stream) == 100;
    sonicDestroyStream(stream);
    return ok;
}

int sonicTestSimpleProcessing(void) {
    sonicStream stream = sonicCreateStream(SAMPLE_RATE, 1);
    short input[100];
    short output[200];
    int i, total, ok;

    if (stream == NULL) {
        return 0;
    }
    for(i=0; i<100; i++) input[i] = i * 100;

    ok = sonicWriteShortToStream(stream, input, 100);
    total = sonicReadShortFromStream(stream, output, 200);
    sonicFlushStream(stream);
    total += sonicReadShortFromStream(stream, output + total, 200 - total);
    sonicDestroyStream(stream);
    return ok && total == 100;
}
