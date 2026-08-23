/* Sonic library
   Copyright 2025
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#include "sonic.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Fuzzer entry point */
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 4) return 0;

    /* Use first few bytes to configure stream */
    int sampleRate = 8000 + (Data[0] * 100); /* 8000 - 33500 */
    int numChannels = (Data[1] % 2) + 1;     /* 1 or 2 */
    
    sonicStream stream = sonicCreateStream(sampleRate, numChannels);
    if (!stream) return 0;

    float speed = 0.5f + (Data[2] / 64.0f); /* ~0.5 - 4.5 */
    float pitch = 0.5f + (Data[3] / 64.0f);
    
    sonicSetSpeed(stream, speed);
    sonicSetPitch(stream, pitch);
    
    /* Feed remaining data as samples */
    /* We treat the uint8_t data as short samples (casting) */
    size_t remaining = Size - 4;
    if (remaining > 0) {
        int numSamples = remaining / (sizeof(short) * numChannels);
        if (numSamples > 0) {
            sonicWriteShortToStream(stream, (short*)(Data + 4), numSamples);
            
            /* Read out some data to exercise processing. maxSamples in
               sonicReadShortFromStream is a per-channel count, so the cap
               passed here must account for numChannels or a stereo stream
               overflows outBuffer (sizeof(outBuffer) / sizeof(short) is
               1024 shorts total, not 1024 per channel). */
            short outBuffer[1024];
            int maxSamplesPerChannel = 1024 / numChannels;
            while (sonicReadShortFromStream(stream, outBuffer,
                                            maxSamplesPerChannel) > 0) {}
        }
    }
    
    sonicFlushStream(stream);
    sonicDestroyStream(stream);
    
    return 0;
}
