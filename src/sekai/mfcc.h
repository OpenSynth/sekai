/*
Copyright (C) 2011-2015 Tobias Platen <tobias@platen-software.de>

This file is part of Sekai.

Sekai is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Sekai is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Sekai.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef WORLD_MFCC_H_
#define WORLD_MFCC_H_
void MFCCCompress(double** spectrogram,int f0_length,int fs, int fft_size,int cepstrum_length,float** mel_cepstrum);
void MFCCDecompress(double** spectrogram,int f0_length,int fs, int fft_size,int cepstrum_length,float** mel_cepstrum, bool is_aperiodicity);
#endif
