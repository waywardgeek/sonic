#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wave.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 11025
#define NUM_SAMPLES 11025
#define NOISE 15

/* Fundamental */
#define MID_FREQ 150
#define FREQ_CHANGE 50

/* Harmonics */
#define NUM_HARMONICS 5
static double harmonic[NUM_HARMONICS] = {4000.0, 4000.0, 2000.0, 0.0, 1000.0};

int main() {
    short samples[NUM_SAMPLES];
    double w = 0.0;
    int i;
    for (i = 0; i < NUM_SAMPLES; i++) {
        double freq = MID_FREQ + FREQ_CHANGE*sin((double)i*2*M_PI/NUM_SAMPLES);
        w += freq*2*M_PI/SAMPLE_RATE;
        samples[i] = 0.0;
        int j;
        for (j = 0; j < NUM_HARMONICS; j++) {
            samples[i] += harmonic[j]*sin((j+1)*w);
        }
        /* Add noise */
        samples[i] += (float)NOISE*rand()/RAND_MAX;
    }
    waveFile file = openOutputWaveFile("output.wav", SAMPLE_RATE, 1);
    writeToWaveFile(file, samples, NUM_SAMPLES);
    closeWaveFile(file);
    return 0;
}
