struct pitch_point
{
	int pos;
	int hz;
};
#define PITCH_RB_SIZE 1024

struct pho_segment
{
	int start;
	int middle;
	int end;
	int vindex;
	float vstart;
	float vmiddle;
	float vend;
};

#define PHO_RB_SIZE 256

class VVDReader;
#include "world/synthesisrealtime.h"
#include <stdio.h>

class VoiceSynth
{
	pitch_point pitch[PITCH_RB_SIZE];
	int pitchWritePos;
	int pitchReadPos;
	
	pho_segment pho[PHO_RB_SIZE];
	int phoWritePos;
	int phoReadPos;
	
	float currentTime;
	
	int catStart;
	int catMiddle;
	int catEnd;
	
	void startCat();
	
	VVDReader* reader;
	float* vvddata;
	float* vvddata2;
	
	//world
	
	int fft_size;//=2048;
    int buffer_size;//=512;
    int samplerate;// = 44100;
    int frame_period;// = 2.5;
    int number_of_pointers;// = 0;
    WorldSynthesizer synth;
    //preallocated input ringbuffer
    double* f0;
    double** spectrogram;
    double** aperiodicity;
    //current ringbuffer index
    int rb;
	
	public:
		VoiceSynth(VVDReader* reader);
		void add_pitch_point(int pos,int hz);
		void addPho(int start,int middle,int end,int vindex,float vstart,float vmiddle,float vend);
		void drain(FILE* outfile);
		void drain2(FILE* outfile);
};
