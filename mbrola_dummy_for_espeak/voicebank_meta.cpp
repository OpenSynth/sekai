#include "voicebank_meta.h"

VoicebankMeta::VoicebankMeta()
{
	segmentMap["_-k"]=0.9;
	segmentMap["k-o"]=0.1;
	segmentMap["o-n1:"]=0.5;
	segmentMap["n1:-n1"]=0.5;
	segmentMap["n1-i"]=0.5;
	segmentMap["i-tS"]=0.8;
	segmentMap["tS-i"]=0.2;
	segmentMap["i-w"]=0.7;
	segmentMap["w-a"]=0.3;
	segmentMap["a-_"]=0.5;
	segmentMap["_-n"]=0.5;
	segmentMap["n-a"]=0.5;
	segmentMap["a-k"]=0.9;
	segmentMap["k-a"]=0.1;
	segmentMap["a-Z"]=0.6;
	segmentMap["Z-i"]=0.4;
	segmentMap["i-m"]=0.5;
	segmentMap["m-a"]=0.5;
	segmentMap["a-d"]=0.7;
	segmentMap["d-e"]=0.2;
	segmentMap["e-s"]=0.7;
	segmentMap["s-u_0"]=0.95;
	segmentMap["u_0-_"]=0.5;
}

int VoicebankMeta::samplerate()
{
	return 16000;
}

float  VoicebankMeta::lookupSegment(std::string segment)
{
	return segmentMap[segment];
}
