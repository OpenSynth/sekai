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
};

#define PHO_RB_SIZE 256

class VoiceSynth
{
	pitch_point pitch[PITCH_RB_SIZE];
	int pitchWritePos;
	int pitchReadPos;
	
	pho_segment pho[PHO_RB_SIZE];
	int phoWritePos;
	int phoReadPos;
	
	float currentTime;
	
	public:
		VoiceSynth();
		void add_pitch_point(int pos,int hz);
		void addPho(int start,int middle,int end);
		void drain();
};
