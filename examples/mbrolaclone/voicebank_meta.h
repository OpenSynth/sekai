#ifndef VOICEBANK_META
#define VOICEBANK_META
#include <string>
#include <map>
class VVDReader;

struct vvd
{
	int index;
	float length;
};

struct segment
{
	int index;
	float start;
	float middle;
	float end;
};

class VoicebankMeta
{
	std::map <std::string,segment*> segments; //TODO upgrade data type
	int index;
	std::map <std::string,vvd*> vvds;
	public:
		VoicebankMeta();
		int samplerate();
		segment* lookupSegment(std::string diph);
		void parseSegmentsFile(std::string fileName);
		void loadVVDs(std::string basePath,VVDReader* reader);
};
#endif
