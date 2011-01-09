# libsonic Home Page

[Download the latest tar-ball from here](download).

The source code repository can be cloned using git:

    $ git clone git://vinux-project.org/sonic

Sonic is a simple algorithm for speeding up or slowing down speech.  However,
it's optimized for speed ups of over 2X, unlike previous algorithms for changing
speech rate.  The sonic library is a very simple ANSI C library that is designed
to easily be integrated into streaming voice applications, like TTS back ends.

The primary motivation behind Sonic is to enable the blind and visually impaired
to improve their productivity with open source speech engines, like espeak.
Sonic can also be used by the sighted.  For example, Sonic can improve the
experience of listening to an audio book on an Android phone.

Sonic is Copyright 2010, 2011, Bill Cox, all rights reserved.  It is released as
open source under the Lesser Gnu Public License version 2.1.  Feel free to contact
me at <waywardgeek@gmail.com>.  One user was concerned about patents.  I believe
the sonic algorithms do not violate any patents, as most of it is very old,
based on [PICOLA](http://keizai.yokkaichi-u.ac.jp/~ikeda/research/picola.html),
and the new part, for greater than 2X speed up, is clearly a capability most
developers ignore, and would not bother to patent.  I will not file any patents
on sonic algorithms, leaving the algorithms free for you to use.

# Using libsonic in your program

The file main.c is the source code for the sonic command-line application.  It
is meant to be useful as example code.  Feel free to copy directly from it into
your application, as main.c is in the public domain.

There are basically two ways to use sonic: batch or stream mode.  The simplest
is batch mode where you pass an entire sound sample to sonic.  All you do is
call one function, like this:

    sonicChangeShortSpeed(samples, numSamples, speed, pitch, volume, sampleRate, numChannels);

This will change the speed and pitch of the sound samples pointed to by samples,
which should be 16-bit signed integers.  Stereo mode is supported, as
is any arbitrary number of channels.  Samples for each channel should be
adjacent in the input array.  Because the samples are modified in-place, be sure
that there is room in the samples array for the speed-changed samples.  In
general, if you are speeding up, rather than slowing down, it will be safe to
have no extra padding.

The other way to use libsonic is in stream mode.  This is more complex, but
allows sonic to be inserted into a sound stream with fairly low latency.  The
current maximum latency in sonic is 31 milliseconds, which is enough to process
two pitch periods of voice as low as 65 Hz.  In general, the latency is equal to
two pitch periods, which is typically closer to 20 milliseconds.

To process a sound stream, you must create a sonicStream object, which contains
all of the state used by sonic.  Sonic should be thread safe, and multiple
sonicStream objects can be used at the same time.  You create a sonicStream
object like this:

    sonicStream stream = sonicCreateStream(sampleRate, numChannels);

When you're done with a sonic stream, you can free it's memory with:

    sonicDestroyStream(stream);

By default, a sonic stream sets the speed, pitch, and volume to 1.0, which means
no change at all to the sound stream.  Sonic detects this case, and simply
copies the input to the output to reduce CPU load.  To change the speed, pitch,
or volume, set the parameters using:

    sonicSetSpeed(stream, speed);
    sonicSetPitch(stream, pitch);
    sonicSetVolume(stream, volume);

These three parameters are floating point numbers.  A speed of 2.0 means to
double speed of speech.  A pitch of 0.95 means to lower the pitch by about 5%,
and a volume of 1.4 means to multiply the sound samples by 1.4, clipping if we
exceed the maximum range of a 16-bit integer.

After setting the sound parameters, you write to the stream like this:

    sonicWriteShortToStream(stream, samples, numSamples);

You read the sped up speech samples from sonic like this:

    samplesRead = sonicReadShortFromStream(stream, outBuffer, maxBufferSize);
    if(samplesRead > 0) {
	/* Do something with the output samples in outBuffer, like send them to
	 * the sound device. */
    }

You may change the speed, pitch, and volume parameters at any time, without
having to flush or create a new sonic stream.

When your sound stream ends, there may be several milliseconds of sound data in
the sonic stream's buffers.  To force sonic to process those samples use:

    sonicFlushStream(stream);

Then, read those samples as above.  That's about all there is to using libsonic.
There are some more functions as a convenience for the user, like
sonicGetSpeed.  Other sound data formats are supported: signed char and float.
If float, the sound data should be between -1.0 and 1.0.  Internally, all sound
data is converted to 16-bit integers for processing.
