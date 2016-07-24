/*
	Sekai - addons for the WORLD speech toolkit
    Copyright (C) 2016 Tobias Platen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 

#ifndef SEKAI_HZOSC_H
#define SEKAI_HZOSC_H

#include <fftw3.h>

#define N_OSC 100
#define FFT_SIZE 2048

class IFFTOsc
{
public:
  double phase[N_OSC];
  double amp[N_OSC];
  double frq[N_OSC];
    IFFTOsc ();
  void processOneFrame ();
  double output_buffer[FFT_SIZE/2];
  int fs;
private:
  float window[FFT_SIZE];
  fftw_complex *spectrum;
  double *waveform;
  fftw_plan inverse_c2r;
  double rover[FFT_SIZE/2];
};

#endif

