#ifndef VOICEBANK_META
#define VOICEBANK_META
#include <string>
#include <map>
class VoicebankMeta
{
	std::map <std::string,float> segmentMap;
	public:
		VoicebankMeta();
		int samplerate();
		float lookupSegment(std::string diph);
};
#endif
