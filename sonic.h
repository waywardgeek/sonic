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
The Sonic Library implements Pitch Based Resampling, which is a new algorithm
invented by Bill Cox for the specific purpose of speeding up speech by high
factors at high quality.  It generates smooth speech at speed up factors as high
as 6X, possibly more.  It is also capable of slowing down speech, and generates
high quality results regardless of the speed up or slow down factor.  For
speeding up speech by 2X or more, the following equation is used:

    newSamples = period/(speed - 1.0)
    scale = 1.0/newSamples;

where period is the current pitch period, determined using AMDF or any other
pitch estimator, and speed is the speedup factor.  If the current position in
the input stream is pointed to by "samples", and the current output stream
position is pointed to by "out", then newSamples number of samples can be
generated with:

    out[t] = (samples[t]*(newSamples - t) + samples[t + period]*t)/newSamples;

where t = 0 to newSamples - 1.

For speed factors < 2X, an algorithm similar to PICOLA is used.  The above
algorithm is first used to double the speed of one pitch period.  Then, enough
input is directly copied from the input to the output to achieve the desired
speed up facter, where 1.0 < speed < 2.0.  The amount of data copied is derived:

    speed = (2*period + length)/(period + length)
    speed*length + speed*period = 2*period + length
    length(speed - 1) = 2*period - speed*period
    length = period*(2 - speed)/(speed - 1)

For slowing down speech where 0.5 < speed < 1.0, a pitch period is inserted into
the output twice, and length of input is copied from the input to the output
until the output desired speed is reached.  The length of data copied is:

    length = period*(speed - 0.5)/(1 - speed)

For slow down factors between 0.5 and 0.5, no data is copied, and an algorithm
similar to high speed factors is used.
*/

/* This specifies the range of voice pitches we try to match. */
#define SONIC_MIN_PITCH 50
#define SONIC_MAX_PITCH 400

struct sonicStreamStruct;
typedef struct sonicStreamStruct *sonicStream;

/* Create a sonic stream.  Return NULL only if we are out of memory and cannot
  allocate the stream. */
sonicStream sonicCreateStream(double speed, int sampleRate);
/* Destroy the sonic stream. */
void sonicDestroyStream(sonicStream stream);
/* Use this to write data to be speed up or down into the stream.  Return 0 if
   memory realloc failed, otherwise 1 */
int sonicWriteToStream(sonicStream stream, float *samples, int numSamples);
/* Use this to read data out of the stream.  Sometimes no data will be
   available, and zero is returned, which is not an error condition. */
int sonicReadFromStream(sonicStream stream, float *samples, int maxSamples);
/* Force the sonic stream to generate output using whatever data it currently
   has.  Zeros will be appended to the input data if there is not enough data
   in the stream's input buffer.  Use this, followed by a final read from the
   stream before destroying the stream. */
int sonicFlushStream(sonicStream stream);
/* Return the number of samples in the output buffer */
int sonicSamplesAvailale(sonicStream stream);
