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

#ifndef SEKAI_COMMON_H
#define SEKAI_COMMON_H

#define LOG10 2.3025850929940459  /*!< natural logarithm of 10 */
#define TWENTY_OVER_LOG10 (20. / LOG10)
#define Np2dB(x) (TWENTY_OVER_LOG10 * (x))
#define dB2Np(x) ((x)/TWENTY_OVER_LOG10)

float interp_linear(float* x,float* y,int nx,float ref);
double interp_linear(double* x,double* y,int nx,double ref);

#endif
