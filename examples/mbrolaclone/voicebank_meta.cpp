#include "voicebank_meta.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <sekai/VVDReader.h>

VoicebankMeta::VoicebankMeta()
{
	index=0;
}

int VoicebankMeta::samplerate()
{
	return 16000;
}

segment* VoicebankMeta::lookupSegment(std::string diph)
{
	return segments[diph];
}


static int split(char* str,int len,int start,char chr)
{
	int i;
	for(i=start;i<len;i++)
	{
		if(str[i]==chr)
		{
			str[i]=0;
			return i;
		}
	}
	return 0;
}

static void wav2vvd(char* str,int len)
{
	int i;
	for(i=0;i<len-2;i++)
	{
		if(str[i]=='w'||str[i+1]=='a'||str[i+2]=='v')
		{
			str[i]='v';
			str[i+1]='v';
			str[i+2]='d';
			return;
		}
	}
}

void VoicebankMeta::parseSegmentsFile(std::string fileName)
{
	char textbuf[1024];
	FILE* f = fopen (fileName.c_str(),"r");
	while(fgets(textbuf, sizeof(textbuf), f))
	{
		textbuf[strlen(textbuf)-1]=0;
		int s1 = split(textbuf, sizeof(textbuf),0,'=');
		int s2 = split(textbuf, sizeof(textbuf),s1,',');
		int s3 = split(textbuf, sizeof(textbuf),s2,',');
		int s4 = split(textbuf, sizeof(textbuf),s3,',');
		
		wav2vvd(textbuf,strlen(textbuf));
		
		
		
		std::string key = textbuf;
		if(vvds.count(key)==0)
		{
			vvd* v = new vvd;
			v->index = index;
			v->length = 0;
			vvds[key] = v;
			index++;
		}
		//add vvd to map
		
		key = textbuf+s1+1;
		if(segments.count(key)==0)
		{
			segment* s = new segment;
			s->start = atoi(textbuf+s2+1)/16000.0;
			s->end=atoi(textbuf+s3+1)/16000.0;
			s->middle=atoi(textbuf+s4+1)/16000.0;
			s->index=index-1;
			segments[key]=s;
		}
		else
		{
			fprintf(stderr,"multiple entries for segment [%s]",textbuf+s1+1);
			abort();
		}
		
		//add segment to map
		
	}
}

void VoicebankMeta::loadVVDs(std::string basePath,VVDReader* reader)
{
	for ( const auto &it : vvds ) {
		std::string vvdFullPath = basePath+"/"+it.first;
        reader->addVVD(vvdFullPath);
        it.second->length = reader->getSelectedLength();
    }
}
