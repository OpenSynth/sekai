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

#include "sekai/OLABuffer.h"

OLABuffer::OLABuffer( int bufferLen ) {

  length = bufferLen; // init the OLA buffer to 0
  rawData = (float *) malloc( length*sizeof(float) );
  memset( rawData, 0, length*sizeof(float) );
  pos = 0; // 1st position is buffer head
  atLoc = 0;
  timeSamples = 0;
}


void OLABuffer::ola( float *frame, int frameLen, float period ) {

  int m,k;
  
  float a = atLoc-((int)atLoc);
  float b = 1-a;
  float nextSample = 0;

  for( k=0; k<frameLen; k++ ) {
    
    m = (pos+((int)atLoc)+k)%length; 	   // we cycle through OLA buffer indices
    if(k<frameLen-1) nextSample=frame[k+1];
    rawData[m] += frame[k]*a+nextSample*b; // and add the current frame at location and apply fractional delay
  }

  atLoc+=period;      //buffer location
  timeSamples+=period;//absolute time


}

void OLABuffer::ola( double *frame, int frameLen, float period ) {

  int m,k;
  
  float a = atLoc-((int)atLoc);
  float b = 1-a;
  float nextSample = 0;

  for( k=0; k<frameLen; k++ ) {
    
    m = (pos+((int)atLoc)+k)%length; 	   // we cycle through OLA buffer indices
    if(k<frameLen-1) nextSample=frame[k+1];
    rawData[m] += frame[k]*a+nextSample*b; // and add the current frame at location and apply fractional delay
  }

  atLoc+=period;      //buffer location
  timeSamples+=period;//absolute time
}

bool OLABuffer::isFilled(int size)
{
  return atLoc>size;
}

int OLABuffer::currentTime()
{
  return timeSamples;
}


void OLABuffer::pop( float *buffer, int bufferLen ) {

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

#ifdef TEST_OLABUFFER
#include <sndfile.h>
int main()
{
	float* frame;
	int frameLen;
	
	SF_INFO info;
	SNDFILE* sf = sf_open("vowel_a.wav",SFM_READ,&info);
	frameLen = info.frames;
	frame = new float[frameLen];
	sf_read_float(sf,frame,frameLen);
	sf_close(sf);
	
	info.frames = 0;
	info.sections = 0;
	info.seekable = 0;
	sf = sf_open("output.wav",SFM_WRITE,&info);
	
	float period = 44100.0/440.0;
	fprintf(stderr,"period = %f\n",period);
	
	#define OUTBUF_LEN 1024
	float outbuf[OUTBUF_LEN];
	
	OLABuffer buffer(2048*8);
	while(buffer.currentTime()<44100)
	{
		
		buffer.ola(frame,frameLen,period);
		while(buffer.isFilled(OUTBUF_LEN*4))
		{
			buffer.pop(outbuf,OUTBUF_LEN);
			sf_write_float(sf,outbuf,OUTBUF_LEN);
		}
	}
	while(buffer.isFilled(OUTBUF_LEN))
	{
		buffer.pop(outbuf,OUTBUF_LEN);
		sf_write_float(sf,outbuf,OUTBUF_LEN);
	}
	
	sf_close(sf);
}
#endif
