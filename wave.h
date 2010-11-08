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

/* Support for reading and writing wave files. */

typedef struct waveFileStruct *waveFile;

waveFile openInputWaveFile(char *fileName, int *sampleRate);
waveFile openOutputWaveFile(char *fileName, int sampleRate);
void closeWaveFile(waveFile file);
int readFromWaveFile(waveFile file, short *buffer, int maxSamples);
int writeToWaveFile(waveFile file, short *buffer, int numSamples);
