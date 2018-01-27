#include "synth.hpp"
#include <math.h>
#include <iostream>
#include <stdlib.h>     /* srand, rand */
#include <sekai/mfcc.h>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sekai/common.h>

#define TWOPI (2*M_PI)

using namespace std;

extern std::string basedir;

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
  
  enabled = false;
  
  reader = new VVDReader();
  int fft_size = 2048;
  fprintf(stderr,"sample_rate=%i\n",samplerate);
  synth = new WorldSynth2(1024*1024*1024,fft_size,samplerate);
  
  //jsoncpp is only used by this test program
  ifstream ifs(basedir+"/voice.json");
  Json::Reader jreader;
  Json::Value arr;
  jreader.parse(ifs, arr); 
    
  for (int i = 0; i < arr.size(); i++){
        std::string vvd = arr[i]["vvd"].asString();
        auto x = arr[i]["x"];
        auto y = arr[i]["y"];
        bool valid = reader->addVVD(basedir+"/"+vvd);
        if(!valid) {
                fprintf(stderr,"invalid vvd\n");
                abort();
        }
        if(x.size()!=y.size())
        {
            fprintf(stderr,"invalid entry in voices.json\n");
            abort();
        }
        segment* seg = new segment;
        seg->count=x.size();
        seg->x=new float[seg->count];
        seg->y=new float[seg->count];
        for (int j = 0; j < x.size(); j++){
                seg->x[j]=x[j].asInt()*0.001;
                seg->y[j]=y[j].asInt()*0.001;
        }
        segments.push_back(seg);
  }
  
  
	
  vvddata = (float*) new char[reader->getFrameSize()];//FIXME add allocator for vvddata
    
}

Synth::~Synth()
{
}

void Synth::noteOn(int notenum, int velocity)
{
  this->notenum = notenum;
  int current_samples = synth->currentTime();//time in samples
  float current_time = current_samples*1.0/samplerate;
  
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
		
  //fprintf(stderr,"select %i\n",reader->selectVVD(current_oto));
		
        
    if(current_oto!=7)
    {
        segment* seg = segments[current_oto];
        float delta = current_time - seg->x[0];
        for(int i=0;i<seg->count;i++)
        {
            seg->x[i]+=delta;
        }
        reader->selectVVD(current_oto);
        enabled = true;
    }
    else
        enabled = false;
  
		
}
void Synth::noteOff(int notenum)
{
    this->notenum = 0;
    enabled=false;
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
    if(notenum) return (float)midi_freq(notenum);
    else return 0.0;
}

segment* Synth::getCurrentSegment(int)
{
    if(enabled)
    {
            return segments[current_oto];
    }
    else return 0;
    
}

void Synth::fill(float* buffer, int size)
{
    int cepstrum_length = 32;//FIXME
    while(1)
    {
 
    int current_samples = synth->currentTime();//time in samples
    float current_time = current_samples*1.0/samplerate;
    
    segment* seg = getCurrentSegment(current_samples);
    
    #if 1
    if(seg && current_time > seg->x[seg->count-1])
    {
        enabled=false;
        seg=0;
    }
    #endif
    
    if(seg) {
        float f0 = getCurrentF0(current_samples);
        
        
        float fractionalIndex=interp_linear(seg->x,seg->y,seg->count,current_time)*1000.0/reader->getFramePeriod();
        reader->getSegment(fractionalIndex,vvddata);
        
        float* mel_cepstrum1 = &vvddata[1];
        float* mel_cepstrum2 = &vvddata[1+cepstrum_length];
        //TODO
        // getCurrentConcatenation(current_time)
        // concatenation
        synth->setFrame(mel_cepstrum1,mel_cepstrum2,cepstrum_length);
        synth->setF0(f0);
    } else {
        synth->setSilence();
    }
    

    synth->doSynth(); //synth one frame
    
    if(synth->isFilled(size+2048)) break;
    
    }
    
    synth->pop(buffer,size);
    
 
}
