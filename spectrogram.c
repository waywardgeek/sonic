/* Sonic library
   Copyright 2016
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <fftw3.h>
#include "spectrogram.h"

/* Deal with lack of M_PI in ANSI C. */
#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

struct sonicSpectrumStruct;
typedef struct sonicSpectrumStruct *sonicSpectrum;

struct sonicSpectrogramStruct {
    sonicSpectrum *spectrums;
    int numSpectrums;
    int allocatedSpectrums;
    double minPower, maxPower;
};

struct sonicSpectrumStruct {
    double *power;
    int numFreqs;  /* Number of frequencies */
};

/* Create an new spectrum. */
static sonicSpectrum sonicCreateSpectrum(sonicSpectrogram spectrogram) {
    sonicSpectrum spectrum = (sonicSpectrum)calloc(1, sizeof(struct sonicSpectrumStruct));
    if(spectrum == NULL) {
        return NULL;
    }
    if(spectrogram->numSpectrums == spectrogram->allocatedSpectrums) {
        spectrogram->allocatedSpectrums <<= 1;
        spectrogram->spectrums = (sonicSpectrum *)realloc(spectrogram->spectrums,
            spectrogram->allocatedSpectrums*sizeof(sonicSpectrum));
    }
    spectrogram->spectrums[spectrogram->numSpectrums++] = spectrum;
    spectrogram->minPower = DBL_MAX;
    spectrogram->maxPower = DBL_MIN;
    return spectrum;
}

/* Destroy the spectrum.  Since the spectrum does not have a pointer to its
   spectrogram, this destructor cannot remove the spectrum from the
   spectrogram.  It is up to the spectrogram destructor to do this. */
static void sonicDestroySpectrum(sonicSpectrum spectrum) {
    if(spectrum == NULL) {
        return;
    }
    if(spectrum->power != NULL) {
        free(spectrum->power);
    }
    free(spectrum);
}

/* Create an empty spectrogram. */
sonicSpectrogram sonicCreateSpectrogram(void) {
    sonicSpectrogram spectrogram = (sonicSpectrogram)calloc(1, sizeof(struct sonicSpectrogramStruct));
    if(spectrogram == NULL) {
        return NULL;
    }
    spectrogram->allocatedSpectrums = 32;
    spectrogram->spectrums = (sonicSpectrum *)calloc(spectrogram->allocatedSpectrums, sizeof(sonicSpectrum));
    if(spectrogram->spectrums == NULL) {
        sonicDestroySpectrogram(spectrogram);
        return NULL;
    }
    return spectrogram;
}

/* Destroy the spectrotram. */
void sonicDestroySpectrogram(sonicSpectrogram spectrogram) {
    if(spectrogram != NULL) {
        if(spectrogram->spectrums != NULL) {
            int i;
            for(i = 0; i < spectrogram->numSpectrums; i++) {
                sonicSpectrum spectrum = spectrogram->spectrums[i];
                sonicDestroySpectrum(spectrum);
            }
            free(spectrogram->spectrums);
        }
        free(spectrogram);
    }
}

/* Create a new bitmap.  This takes ownership of data. */
sonicBitmap sonicCreateBitmap(unsigned char *data, int numRows, int numCols) {
    sonicBitmap bitmap = (sonicBitmap)calloc(1, sizeof(struct sonicBitmapStruct));
    if(bitmap == NULL) {
        return NULL;
    }
    bitmap->data = data;
    bitmap->numRows = numRows;
    bitmap->numCols = numCols;
    return bitmap;
}

/* Destroy the bitmap. */
void sonicDestroyBitmap(sonicBitmap bitmap) {
    if(bitmap == NULL) {
        return;
    }
    if(bitmap->data != NULL) {
        free(bitmap->data);
    }
    free(bitmap);
}

/* Overlap-add the two pitch periods using a Hann window.  Caller must free the result. */
static void computeOverlapAdd(short* samples, int period, double *ola_samples) {
    int i;
    for(i = 0; i < period; i++) {
        double sinx = sin(M_PI*i/2.0);
        ola_samples[i] = sinx*samples[i] + (1.0 - sinx)*samples[i + period];
    }
}

/* Compute the amplitude of the fftw_complex number. */
static double magnitude(fftw_complex c) {
    return sqrt(c[0]*c[0] + c[1]*c[1]);
}

/* Add two pitch periods worth of samples to the spectrogram.  There must be
   2*period samples.  Time should advance one pitch period for each call to
   this function. */
void sonicAddPitchPeriodToSpectrogram(sonicSpectrogram spectrogram, short *samples, int period) {
    sonicSpectrum spectrum = sonicCreateSpectrum(spectrogram);
    /* TODO: convert to fixed-point */
    double in[period];
    fftw_complex out[period/2 + 1];
    spectrum->power = (double*)calloc(period/2, sizeof(double));
    computeOverlapAdd(samples, period, in);
    fftw_plan p = fftw_plan_dft_r2c_1d(period, in, out, FFTW_ESTIMATE);
    fftw_execute(p);
    int i;
    /* Start at 1 to skip the DC power, which is just noise. */
    for(i = 1; i < period/2 + 1; ++i) {
        double m = magnitude(out[i]);
        spectrum->power[i - 1] = m*m;
    }
    fftw_destroy_plan(p);
}

/* Linearly interpolate the power at a given position in the spectrogram. */
static double interpolateSpectrum(sonicSpectrum spectrum, int row, int numRows) {
    /* Flip the row so that we show lowest frequency on the bottom. */
    row = numRows - row - 1;
    /* Does it fall exactly on a row? If not, what row is it between?  */
    int topIndex = spectrum->numFreqs*row/numRows;
    int remainder = spectrum->numFreqs*row - topIndex*numRows;
    double topPower = spectrum->power[topIndex];
    if(remainder == 0) {
        /* Falls exactly on one row. */
        return topPower;
    }
    double bottomPower = spectrum->power[topIndex + 1];
    double position = (double)remainder/numRows;
    return (1.0 - position)*topPower + position*bottomPower;
}


/* Linearly interpolate the power at a given position in the spectrogram. */
static double interpolateSpectrogram(sonicSpectrogram spectrogram, int row, int col,
        int numRows, int numCols) {
    /* Does it fall exactly on a column? If not, what columns is it between?  */
    int leftIndex = spectrogram->numSpectrums*col/numCols;
    int remainder = spectrogram->numSpectrums*col - leftIndex*numCols;
    sonicSpectrum leftSpectrum = spectrogram->spectrums[leftIndex];
    if(remainder == 0) {
        /* Falls exactly on one column. */
        return interpolateSpectrum(leftSpectrum, row, numRows);
    }
    sonicSpectrum rightSpectrum = spectrogram->spectrums[leftIndex + 1];
    double leftPower = interpolateSpectrum(leftSpectrum, row, numRows);
    double rightPower = interpolateSpectrum(rightSpectrum, row, numRows);
    double position = (double)remainder/numCols;
    return (1.0 - position)*leftPower + position*rightPower;
}

/* Convert the spectrogram to a bitmap.  The returned array must be freed by
   the caller.  It will be rows*cols in size.  The pixels are written top row
   to bottom, and each row is left to right.  So, the pixel in the 5th row from
   the top, in the 18th column from the left in a 32x128 array would be in
   position 128*4 + 18.  NULL is returned if calloc fails to allocate the
   memory. */
sonicBitmap sonicConvertSpectrogramToBitmap(sonicSpectrogram spectrogram, int numRows, int numCols) {
    unsigned char *data = (unsigned char*)calloc(numRows*numCols, sizeof(unsigned char));
    if(data == NULL) {
        return NULL;
    }
    int row, col;
    unsigned char *p = data;
    for(row = 0; row < numRows; row++) {
        for(col = 0; col < numCols; col++) {
            double power = interpolateSpectrogram(spectrogram, row, col, numRows, numCols);
            double minPower = spectrogram->minPower;
            double maxPower = spectrogram->maxPower;
            if(power < minPower && power > maxPower) {
                fprintf(stderr, "Power outside min/max range\n");
                exit(1);
            }
            double range = maxPower - minPower;
            int value = (unsigned char)(((power - minPower)/range)*256);
            if(value == 256) {
                value = 255;
            }
            *p++ = value;
        }
    }
    return sonicCreateBitmap(data, numRows, numCols);
}
