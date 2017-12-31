
/*
  
  Author: Harry van Haaren
  E-mail: harryhaaren@gmail.com
  Copyright (C) 2010 Harry van Haaren
  Copyright (C) 2015 Tobias Platen
 
  
  PrintSynth is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  PrintJackMidi is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with PrintJackMidi.  If not, see <http://www.gnu.org/licenses/>. */


#include "jack.hpp"
#include "midi.h"
#include <unistd.h>


using namespace std;

Jack::Jack()
{ 
  std::cout << "Jack()" << std::flush;
  
  if ((client = jack_client_open("SampleMidiSynth", JackNullOption, NULL)) == 0)
    {
      std::cout << "jack server not running?" << std::endl;
    }
  
  synth.init(jack_get_sample_rate(client),jack_get_buffer_size(client));
  bufferSize = jack_get_buffer_size(client);
  
  inputPort  = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  outputPort = jack_port_register (client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  
  inputBuffer = jack_ringbuffer_create(4096);
  outputBuffer = jack_ringbuffer_create(4096*sizeof(float));
  
  
  
  jack_set_process_callback (client, staticProcess, static_cast<void*>(this));
  
  
  std::cout << "\t\t\tDone!" << std::endl;
  
  
  
}

Jack::~Jack()
{
  std::cout << "~Jack()" << std::endl;
}

void Jack::activate()
{
  std::cout << "activate()" << std::flush;
  
  if (jack_activate(client) != 0)
    {
      std::cout<<  "cannot activate client" << std::endl;
      return;
    }
  std::cout << "\t\tDone!" << std::endl;
  
  jack_connect(client,"a2j:LPK25 [20] (capture): LPK25 MIDI 1","SampleMidiSynth:midi_in");
  jack_connect(client,"SampleMidiSynth:audio_out","system:playback_1");
  jack_connect(client,"SampleMidiSynth:audio_out","system:playback_2");
  
  std::thread* thr = new std::thread(Jack::staticWorkerThread,this); 
  
}

int Jack::staticProcess(jack_nframes_t nframes, void *arg)
{
  return static_cast<Jack*>(arg)->process(nframes);
}

int Jack::process(jack_nframes_t nframes)
{
  jack_midi_event_t in_event;
  jack_nframes_t event_index = 0;
  jack_position_t         position;
  jack_transport_state_t  transport;
  
  // get the port data
  void* inputPortBuf = jack_port_get_buffer( inputPort, nframes );
  void* outputPortBuf = jack_port_get_buffer( outputPort, nframes );
 
  jack_nframes_t event_count = jack_midi_get_event_count(inputPortBuf);
  if(event_count > 0)
    {
      for(int i=0; i<event_count; i++)
	{
	  jack_midi_event_get(&in_event, inputPortBuf, i);
                
	  unsigned char event[4];
	  event[0]=0;       
	  event[1]=in_event.buffer[0];
	  event[2]=in_event.buffer[1];
	  event[3]=in_event.buffer[2];
	  if(jack_ringbuffer_write_space(inputBuffer)>4)
	    {
	      jack_ringbuffer_write(inputBuffer,(char*)event,4);
	    }        
                
                   
	}   
    }
  
  jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (outputPort, nframes);
  
  if(jack_ringbuffer_read_space(outputBuffer)>nframes*sizeof(float)*2)
    {
      jack_ringbuffer_read(outputBuffer,(char*)out,nframes*sizeof(float));
    }
  
  
  
  
  return 0;
}

void Jack::staticWorkerThread(Jack* self)
{
  fprintf(stderr,"worker thread - non realtime\n");
  while(1)
    {
      if(jack_ringbuffer_read_space(self->inputBuffer)>=4) {
	unsigned char event[4];
	jack_ringbuffer_read(self->inputBuffer,(char*)event,4);
	int sb=event[1];
	int notenum = event[2];
	int velocity = event[3];
			
	if(MIDI_STATUS(sb)==MIDI_NOTEOFF || (MIDI_STATUS(sb)==MIDI_NOTEON && velocity==0)) self->synth.noteOff(notenum);
	else if(MIDI_STATUS(sb)==MIDI_NOTEON) self->synth.noteOn(notenum,velocity); 
      }
      if(jack_ringbuffer_write_space(self->outputBuffer)>=self->bufferSize*sizeof(float))
	{
	  float buffer[self->bufferSize];
	  self->synth.fill(buffer,self->bufferSize);
	  jack_ringbuffer_write(self->outputBuffer,(char*)buffer,self->bufferSize*sizeof(float));
	}
      usleep(1000);
    }
}
