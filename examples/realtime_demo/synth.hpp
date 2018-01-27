#ifndef SYNTH
#define SYNTH
#include <sekai/VVDReader.h>
#include <sekai/WorldSynth2.h>
#include <vector>

struct segment
{
    float* x;
    float* y;
    int count;
};

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
        //synth
        int samplerate;
        int buffer_size;
		WorldSynth2* synth;
		VVDReader* reader;
        float* vvddata;
        
        //input handling
        int notenum;
        int current_oto;
        std::vector<segment*> segments;
        bool enabled;
        
        segment* getCurrentSegment(int pos);
        float getCurrentF0(int pos);
        
    
};
#endif
