#include "voice_synth.h"
#include <stdio.h>
#include <string.h>

VoiceSynth::VoiceSynth()
{
	pitchReadPos=0;
	pitchWritePos=0;
	
	phoReadPos=0;
	phoWritePos=0;
	
	memset(pitch,0,sizeof(pitch));
	currentTime=0;
	
}

void VoiceSynth::add_pitch_point(int pos,int hz)
{
	pitch[pitchWritePos].pos = pos;
	pitch[pitchWritePos].hz = hz;
	pitchWritePos = (pitchWritePos+1) % PITCH_RB_SIZE;
}

void VoiceSynth::addPho(int start,int middle,int end)
{
	//fprintf(stderr,"pho: %i %i %i\n",start,middle,end);
	pho[phoWritePos].start=start;
	pho[phoWritePos].middle=middle;
	pho[phoWritePos].end=end;
	phoWritePos = (phoWritePos+1) % PHO_RB_SIZE;
}

void VoiceSynth::drain()
{
	while(1)
	{
		int pitchReadPos1 = (pitchReadPos+1) % PITCH_RB_SIZE;
		int phoReadPos1 = (phoReadPos+1) % PHO_RB_SIZE;
		if(pitch[pitchReadPos1].pos==0) return;
		if(pho[phoReadPos1].start==0) return;
		if(currentTime*1000>pitch[pitchReadPos1].pos) {
			pitchReadPos = (pitchReadPos+1);
			pitchReadPos1 = (pitchReadPos+1) % PITCH_RB_SIZE;
		}
		if(currentTime*1000>pho[phoReadPos1].start)
		{
			phoReadPos = (phoReadPos+1) % PHO_RB_SIZE;
			
			fprintf(stderr,"next pho\n");
			fprintf(stderr,"vvdStart=%i vvdStart1=%i\n",pho[phoReadPos].start,pho[phoReadPos1].start);
		}
		
		if(currentTime==0)
		{
			fprintf(stderr,"first pho\n");
			fprintf(stderr,"vvdStart=%i vvdStart1=%i\n",pho[phoReadPos].start,pho[phoReadPos1].start);
		}
		
		float f0=0;
		
		if(pitch[pitchReadPos].hz==0){
			 f0=0;
		}
		else
		{
			float left = pitch[pitchReadPos].hz;
			float right = pitch[pitchReadPos1].hz;
			
			if(right==0)
			{
				f0 = left;
			}
			else
			{
				int len = pitch[pitchReadPos1].pos - pitch[pitchReadPos].pos;
				float a = 1.0*(pitch[pitchReadPos1].pos-1000.0*currentTime)/len; 
				f0 = pitch[pitchReadPos1].hz*(1.0-a)+pitch[pitchReadPos].hz*a;
			}
		}
		fprintf(stderr,"currentTime=%f f0=%f\n",currentTime,f0);
		
		
		
		currentTime+=0.005;
	}
}
