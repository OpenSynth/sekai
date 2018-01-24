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
  //FIXME
}

void Synth::init(int samplerate,int buffer_size)
{
  this->samplerate = samplerate;
  this->buffer_size = buffer_size;
  
  reader = new VVDReader();
  reader->addVVD("do.vvd");		//0
  reader->addVVD("re.vvd");		//1
  reader->addVVD("mi.vvd");		//2
  reader->addVVD("fa.vvd");		//3
  reader->addVVD("sol.vvd");		//4
  reader->addVVD("la.vvd");		//5
  reader->addVVD("si.vvd");		//6
  reader->addVVD("silence.vvd");	//7
  current_oto  = 7;
	
  vvddata = (float*) new char[reader->getFrameSize()];//FIXME add allocator for vvddata
    
}

Synth::~Synth()
{
}

void Synth::noteOn(int notenum, int velocity)
{

  //lastnote=this->notenum;
 
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
		
  fprintf(stderr,"select %i\n",reader->selectVVD(current_oto));
		
  //fprintf(stderr,"noteOn: %i %i\n",notenum,velocity);
		
}
void Synth::noteOff(int notenum)
{
    this->notenum = 0;
#if 0
  if(notenum==this->notenum)
    {
      lastnote=this->notenum;
      this->notenum=0;
      current_frame=0;
      current_oto = 7;
    }
  fprintf(stderr,"select %i\n",vvdreader->selectVVD(current_oto));
#endif
}


float Synth::getCurrentF0(int pos)
{
    (void) pos;
    if(notenum) return midi_freq(notenum);
    else return 0;
}

segment* Synth::getCurrentSegment(int)
{
    return 0;
}

void Synth::fill(float* buffer, int size)
{
    int cepstrum_length = 32;//FIXME
    while(1)
    {
 
    float current_time = synth->currentTime();//time in samples
    //TODO: 1
    segment* seg = getCurrentSegment(current_time);
    
    

    if(seg) {
        float f0 = getCurrentF0(current_time);
        synth->setF0(f0);
        float fractionalIndex=0;//FIXME
        reader->getSegment(fractionalIndex,vvddata);
        
        float* mel_cepstrum1 = &vvddata[1];
        float* mel_cepstrum2 = &vvddata[1+cepstrum_length];
        //TODO: 2
        // getCurrentConcatenation(current_time)
        //concatenation
        TODO: synth->setFrame(mel_cepstrum1,mel_cepstrum2,cepstrum_length);
    } else {
        synth->setSilence();
    }
    

    synth->doSynth(); //synth one frame
    
    if(synth->isFilled(size*2)) break;
    
    }
    
    synth->pop(buffer,size);
    
 
}
