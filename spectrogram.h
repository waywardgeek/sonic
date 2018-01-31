/* Sonic library
   Copyright 2016
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#ifndef SPEEDY_SONIC2_SPECTROGRAM_H_
#define SPEEDY_SONIC2_SPECTROGRAM_H_

/*
This code generates high quality spectrograms from sound samples, using
Time-Aliased-FFTs as described at:

    https://github.com/waywardgeek/spectrogram

Basically, two adjacent pitch periods are overlap-added to create a sound
sample that accurately represents the speech sound at that moment in time.
This set of samples is converted to a spetral line using an FFT, and the result
is saved as a single spectral line at that moment in time.  The resulting
spectral lines vary in resolution (it is equal to the number of samples in the
pitch period), and the spacing of spectral lines also varies (proportional to
the numver of samples in the pitch period).

To generate a bitmap, linear interpolation is used to render the grayscale
value at any particular point in time and frequency.
*/

#define SONIC_MAX_SPECTRUM_FREQ 5000

struct sonicSpectrogramStruct;
typedef struct sonicSpectrogramStruct* sonicSpectrogram;

/* sonicBitmap objects represent spectrograms as grayscale bitmaps where each
   pixel is from 0 (black) to 255 (white).  Bitmaps are rows*cols in size.
   Rows are indexed top to bottom and columns are indexed left to right */
struct sonicBitmapStruct {
  unsigned char* data;
  int numRows;
  int numCols;
};

typedef struct sonicBitmapStruct* sonicBitmap;

/* Create an empty spectrogram. */
sonicSpectrogram sonicCreateSpectrogram(int sampleRate);
/* Destroy the spectrotram. */
void sonicDestroySpectrogram(sonicSpectrogram spectrogram);
/* Add two pitch periods worth of samples to the spectrogram.  There must be
   2*period samples.  Time should advance one pitch period for each call to
   this function. */
void sonicAddPitchPeriodToSpectrogram(sonicSpectrogram spectrogram,
                                      short* samples, int period,
                                      int numChannels);
/* Convert the spectrogram to a bitmap. */
sonicBitmap sonicConvertSpectrogramToBitmap(sonicSpectrogram spectrogram,
                                            int numRows, int numCols);
/* Create a new bitmap.  This takes ownership of data. */
sonicBitmap sonicCreateBitmap(unsigned char* data, int numRows, int numCols);
/* Destroy the bitmap. */
void sonicDestroyBitmap(sonicBitmap bitmap);
/* Write a PGM image file, which is 8-bit grayscale and looks like:
    P2
    # CREATOR: libsonic
    640 400
    255
    ...
*/
int sonicWritePGM(sonicBitmap bitmap, char* fileName);

#endif /* SPEEDY_SONIC2_SPECTROGRAM_H_ */
