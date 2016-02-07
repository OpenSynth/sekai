/* Copyright 2013 Tobias Platen */
/*  This file is part of Sekai.

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

#ifndef WORLD_EPR_H_
#define WORLD_EPR_H_

typedef struct {
 double gaindb;
 double slope;
 double slopedepthdb;
} EprSourceParams;

void EprSourceEstimate(double* spectrogram,int fft_size,int fs,double f0,EprSourceParams* params);

typedef struct
{
	//set by the user
	double f;
	double bw;
	double gain_db;

	//needs to be updated by calling EprResonanceUpdate if any user value is changed
	double a;
	double b;
	double c;
	double norm;

} EprResonance;

int EprVocalTractEstimate(double* spectrogram,int fft_size,int fs,double f0,EprSourceParams* params,double* residual,EprResonance* res, int nres);

double EprResonanceAtFrequency(EprResonance* me,double f,int fs);
void EprResonanceUpdate(EprResonance* me,int fs);

double EprAtFrequency(EprSourceParams* params,double f,int fs,EprResonance* res,int n_res);

#endif
