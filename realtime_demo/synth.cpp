#include "synth.hpp"
#include <math.h>
#include <iostream>
#include <stdlib.h>     /* srand, rand */
#include <sekai/mfcc.h>

#define TWOPI (2*M_PI)

double midi_freq(float m) {
    /* converts a MIDI note number to a frequency
       <http://en.wikipedia.org/wiki/MIDI_Tuning_Standard> */
    return 440 * pow(2, (double)(m-69.0)/12.0);
}


Synth::Synth()
{
	notenum=0;
	current_frame=0;
	
	frame_period = 5.0;
	number_of_pointers = 80;
	rb=0;
}

void Synth::init(int samplerate,int buffer_size)
{
	this->samplerate = samplerate;
	this->buffer_size = buffer_size;
	fft_size = 2048;
	
	InitializeSynthesizer(samplerate, frame_period, fft_size,
    buffer_size, number_of_pointers, &rtsynth);
    
    f0 = new double[number_of_pointers];
    spectrogram = new double*[number_of_pointers];
    aperiodicity = new double*[number_of_pointers];
    for(int i=0;i<number_of_pointers;i++)
    {
		spectrogram[i] = new double[fft_size/2+1];
		aperiodicity[i] = new double[fft_size/2+1];
	}
	
	vvdreader = new VVDReader();
	fprintf(stderr,"load vvd containing japanese vowels %i\n",vvdreader->addVVD("あいうえお.vvd"));
	
	fprintf(stderr,"select %i\n",vvdreader->selectVVD(0));
	
	vvddata = (void*) new char[vvdreader->getFrameSize()];
    
}

Synth::~Synth()
{
}

void Synth::noteOn(int notenum, int velocity)
{
		lastnote=this->notenum;
		this->notenum = notenum;
		note_time=0;
		
		fprintf(stderr,"noteOn: %i %i\n",notenum,velocity);
		
}
void Synth::noteOff(int notenum)
{
		if(notenum==this->notenum)
		{
			lastnote=this->notenum;
			this->notenum=0;
			current_frame=0;
		}
}


void Synth::fill(float* buffer, int size)
{
		for(int i=0;i<size;i++)
			buffer[i]=0;
			
		while(1)
		{
			
			float current_fill = rtsynth.head_pointer - rtsynth.current_pointer2;
			float fill_max = rtsynth.number_of_pointers;
			float fill_ratio = current_fill/fill_max;
	
			//fprintf(stderr,"fill ratio %f\n",fill_ratio);
			
			if(fill_ratio>0.5) break;
		
		if(notenum>0)
		{
			f0[rb] = midi_freq(notenum);
			note_time++;
			
			double offset = 0.1;
			double factor = 0.0032;
			double end=1;
			
			double pos = offset+factor*note_time;
			
			
			float fractIndex=pos*1000/frame_period;
			
			bool valid = vvdreader->getSegment(fractIndex,vvddata);
			
			if(!valid) {
				fprintf(stderr,"invalid segment %f\n",pos);
				notenum = 0;
			}
			
			if(pos>end) notenum = 0;
		}
		else
		{
			f0[rb] = 0;
			float fractIndex = 5;
			bool valid = vvdreader->getSegment(fractIndex,vvddata);
			if(!valid) fprintf(stderr,"invalid segment 2\n");
		}
		
		int cepstrum_length=vvdreader->getCepstrumLength();
		float* tmp = (float*)vvddata;
		float* mel_cepstrum1 = &tmp[1];
		float* mel_cepstrum2 = &tmp[1+cepstrum_length];
		
		MFCCDecompress(&spectrogram[rb],1,samplerate, fft_size,cepstrum_length,&mel_cepstrum1,false);
		MFCCDecompress(&aperiodicity[rb],1,samplerate, fft_size,cepstrum_length,&mel_cepstrum2,true);
		AddParameters(&f0[rb], 1, &spectrogram[rb], &aperiodicity[rb],&rtsynth);
		rb = (rb+1) % number_of_pointers;
		
		} //end fill
		
		int ret = Synthesis2(&rtsynth);
		//int l = IsLocked(&rtsynth);
		
		
			
		if(ret)
		{
			for(int i=0;i<size;i++)
				buffer[i]=rtsynth.buffer[i];
		}
		else
			fprintf(stderr,"fill buffer\n");
		
}
