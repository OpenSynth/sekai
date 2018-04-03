/*
    Copyright 2011 University of Mons

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

#ifndef OLABUFFER_H
#define OLABUFFER_H

#include <cstdlib>
#include <cstring>

class OLABuffer {
  public:
    OLABuffer(int bufferLen);
    void ola(float *frame, int frameLen, float period);
    void ola(double *frame, int frameLen, float period);
    void pop(float *buffer, int bufferLen);
    bool isFilled(int size);
    int currentTime();
    void reset();
  protected:
    float *rawData;
    int length, pos;
    float timeSamples;
    float atLoc;
};

#endif
