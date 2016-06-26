#ifndef SYNTH
#define SYNTH
#include <world/synthesisrealtime.h>
#include <sekai/VVDReader.h>
class Synth
{
	public:
	Synth();
	~Synth();
	void init(int samplerate,int buffer_size);
	void noteOn(int notenum,int velocity);
	void noteOff(int notenum);
	void fill(float* samples,int count);
	private:
		WorldSynthesizer rtsynth;
		int notenum;
		int lastnote;
		int current_frame;
		int current_oto;
		float amp;
		///
		int rb;
		int samplerate;
		double frame_period;
		int fft_size;
		int buffer_size;
		int number_of_pointers;
		int note_time;
		
		double* f0;
		double** spectrogram;
		double** aperiodicity;
		
		VVDReader* vvdreader;
		void* vvddata;
};
#endif
