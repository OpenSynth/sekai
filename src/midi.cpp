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

#include <math.h>
const static double A4Frequency = 440.0;
const static double A4Note = 69.0;

//based on https://github.com/haruneko/SimpleSynthesizer/blob/master/core/org/stand/util/MusicalNote.{h,cpp}

double frequencyFromNote(float note)
{
  if(note==0) return 0;
  return pow(2.0, (note - A4Note) / 12.0) * A4Frequency;
}

float noteFromFrequency(double frequency)
{
  if(frequency==0) return 0;
  return A4Note + 12.0 * log2(frequency / A4Frequency);
}


