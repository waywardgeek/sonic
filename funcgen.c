#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wave.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 22050
#define NUM_SAMPLES 22050
#define NOISE 0

/* Fundamental */
#define MID_FREQ 150
#define FREQ_CHANGE 50
#define MAX_FREQ 2000.0

/* Harmonics */
#define NUM_HARMONICS 20
static double harmonic[NUM_HARMONICS] = {1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0,
    1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0};
/* The phase seems to make no difference in how it sounds to me. */
static double phase[NUM_HARMONICS] = {0.0, 1.0, 2.0, 5.5, 4.1234, 2.8, 6.0, 4.5, 2.67, 0.543,
    0.01, 1.234, 2.345, 3.456, 5.9, 4.8, 0.89, 1.33, 3.8, 3.14159};
/* static double phase[NUM_HARMONICS] = {0.0,}; */

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
            double amplitude = harmonic[j];
            if ((j+1)*freq > MAX_FREQ) {
                if ((j)*freq >= MAX_FREQ) {
                    amplitude = 0.0;
                } else {
                    double ratio = ((j+1)*freq - MAX_FREQ)/freq;
                    amplitude *= 1.0 - ratio;
                }
            }
            samples[i] += amplitude*sin((j+1)*w + phase[j]);
        }
        /* Add noise */
        samples[i] += (float)NOISE*rand()/RAND_MAX;
    }
    waveFile file = openOutputWaveFile("output.wav", SAMPLE_RATE, 1);
    writeToWaveFile(file, samples, NUM_SAMPLES);
    closeWaveFile(file);
    return 0;
}
