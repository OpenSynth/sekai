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
  vvdreader->addVVD("do.vvd");		//0
  vvdreader->addVVD("re.vvd");		//1
  vvdreader->addVVD("mi.vvd");		//2
  vvdreader->addVVD("fa.vvd");		//3
  vvdreader->addVVD("sol.vvd");		//4
  vvdreader->addVVD("la.vvd");		//5
  vvdreader->addVVD("si.vvd");		//6
  vvdreader->addVVD("silence.vvd");	//7
  current_oto  = 7;
	
  fprintf(stderr,"select %i\n",vvdreader->selectVVD(current_oto));
	
  vvddata = (void*) new char[vvdreader->getFrameSize()];
    
}

Synth::~Synth()
{
}

void Synth::noteOn(int notenum, int velocity)
{
  lastnote=this->notenum;
  this->notenum = notenum;
  //amp = ((float)velocity)/127;
  amp = 1;
  note_time=0;
		
		
  switch(notenum % 12)
    {
    case 0:
      current_oto = 0;
      break;
    case 2:
      current_oto = 1;
      break;
    case 4:
      current_oto = 2;
      break;
    case 5:
      current_oto = 3;
      break;
    case 7:
      current_oto = 4;
      break;
    case 9:
      current_oto = 5;
      break;
    case 11:
      current_oto = 6;
      break;
    default:
      current_oto = 7;
    }
		
  fprintf(stderr,"select %i\n",vvdreader->selectVVD(current_oto));
		
  //fprintf(stderr,"noteOn: %i %i\n",notenum,velocity);
		
}
void Synth::noteOff(int notenum)
{
  if(notenum==this->notenum)
    {
      lastnote=this->notenum;
      this->notenum=0;
      current_frame=0;
      current_oto = 7;
    }
  fprintf(stderr,"select %i\n",vvdreader->selectVVD(current_oto));
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
			
	  double offset = 0;
	  double factor = 0.0052;
	  double end=1;
			
	  double pos = offset+factor*note_time;
			
	  float fractIndex=pos*1000/frame_period;
			
	  if(current_oto==7) fractIndex = 5;
			
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
	buffer[i]=rtsynth.buffer[i]*amp;
    }
  else
    fprintf(stderr,"fill buffer\n");
		
}
