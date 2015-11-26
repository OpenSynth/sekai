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

#include "obOlaBuffer.h"



obOlaBuffer::obOlaBuffer( int bufferLen ) {

  length = bufferLen; // init the OLA buffer to 0
  rawData = (float *) malloc( length*sizeof(float) );
  memset( rawData, 0, length*sizeof(float) );
  pos = 0; // 1st position is buffer head
  atLoc = 0;
  timeSamples = 0;
}


void obOlaBuffer::ola( float *frame, int frameLen, int period ) {

  int m,k;

  for( k=0; k<frameLen; k++ ) {
    
    m = (pos+((int)atLoc)+k)%length; // we cycle through OLA buffer indices
    rawData[m] += frame[k]; // and add the current frame at location
  }

  atLoc+=period;      //buffer location
  timeSamples+=period;//absolute time


}

bool obOlaBuffer::isFilled(int size)
{
  return atLoc>size;
}

int obOlaBuffer::currentTime()
{
  return timeSamples;
}


void obOlaBuffer::pop( float *buffer, int bufferLen ) {

  if( pos+bufferLen < length ) {
    
    // transfer samples to out in one block
    memcpy( buffer, &rawData[pos], bufferLen*sizeof(float) );
    memset( &rawData[pos], 0, bufferLen*sizeof(float) );
        
  } else {
    
    // remaining bit
    int remain = length-pos;
        
    // transfer/clean from pos to end
    memcpy( buffer, &rawData[pos], remain*sizeof(float) );
    memset( &rawData[pos], 0, remain*sizeof(float) );
        
    // transfer/clean from beginning to remaining bit
    memcpy( &buffer[remain], rawData, (bufferLen-remain)*sizeof(float) );
    memset( rawData, 0, (bufferLen-remain)*sizeof(float) );
  }
    
  // finally move pos to next
  pos = (pos+bufferLen)%length;

  atLoc -= bufferLen;

	
}
