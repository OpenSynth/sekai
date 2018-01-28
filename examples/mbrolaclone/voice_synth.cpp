#include "voice_synth.h"
#include <stdio.h>
#include <string.h>
#include <sekai/VVDReader.h>
#include <sekai/common.h>
#include <sekai/midi.h>
#include <sekai/mfcc.h>
#include "world/cheaptrick.h"


inline int MyMax(int x, int y) {
    return x > y ? x : y;
}
inline double MyMax(double x, double y) {
    return x > y ? x : y;
}
inline int MyMin(int x, int y) {
    return x < y ? x : y;
}
inline double MyMin(double x, double y) {
    return x < y ? x : y;
}

void wavwrite_buffer(FILE* fp,int x_length,double* x)
{

    int16_t tmp_signal;
    for (int i = 0; i < x_length; ++i) {
        tmp_signal = static_cast<int16_t>(MyMax(-32768,
                                                MyMin(32767, static_cast<int>(x[i] * 32767))));
        fwrite(&tmp_signal, 2, 1, fp);
    }
    fflush(stdout);
}

VoiceSynth::VoiceSynth(VVDReader* reader)
{
	this->reader=reader;
	fprintf(stderr,"frameSize %i\n",reader->getFrameSize());
	vvddata  = (float*) malloc(reader->getFrameSize());
	vvddata2 = (float*) malloc(reader->getFrameSize());
	
	pitchReadPos=0;
	pitchWritePos=0;
	
	phoReadPos=0;
	phoWritePos=0;
	
	catStart=0;
	catMiddle=0;
	catEnd=0;
	
	
	memset(pitch,0,sizeof(pitch));
	currentTime=0;
	
	samplerate = reader->getSamplerate();
	frame_period = reader->getFramePeriod();
    #if 0
    //this has changed
	CheapTrickOption option = {0};
	InitializeCheapTrickOption(&option);
	fft_size = GetFFTSizeForCheapTrick(samplerate,&option);
    #else
    fft_size = 0;
    #endif
    
	buffer_size = 512;
	number_of_pointers = 100;
	
    #if 0
	InitializeSynthesizer(samplerate, frame_period, fft_size,
                              buffer_size, number_of_pointers, &synth);
    #endif

        //alloc events

    f0 = new double[number_of_pointers];
    spectrogram = new double*[number_of_pointers];
    aperiodicity = new double*[number_of_pointers];
    for(int i=0;i<number_of_pointers;i++)
    {
         spectrogram[i]  = new double[fft_size/2+1];
         aperiodicity[i] = new double[fft_size/2+1];
    }
    rb=0;
	
}

void VoiceSynth::add_pitch_point(int pos,int hz)
{
	pitch[pitchWritePos].pos = pos;
	pitch[pitchWritePos].hz = hz;
	pitchWritePos = (pitchWritePos+1) % PITCH_RB_SIZE;
}

void VoiceSynth::addPho(int start,int middle,int end,int vindex,float vstart,float vmiddle,float vend)
{
	pho[phoWritePos].start=start;
	pho[phoWritePos].middle=middle;
	pho[phoWritePos].end=end;
	pho[phoWritePos].vindex=vindex;
	pho[phoWritePos].vstart=vstart;
	pho[phoWritePos].vmiddle=vmiddle;
	pho[phoWritePos].vend=vend;
	phoWritePos = (phoWritePos+1) % PHO_RB_SIZE;
}

void VoiceSynth::startCat()
{
	int phoReadPos1 = (phoReadPos+1) % PHO_RB_SIZE;
	if(pho[phoReadPos1].middle==pho[phoReadPos1].start &&
	   pho[phoReadPos1].middle==pho[phoReadPos1].end )
	{
		catStart=0;
		catMiddle=0;
		catEnd=0;
	}
	else
	{
		catStart  = pho[phoReadPos].middle;
		catMiddle = pho[phoReadPos].end;
		catEnd    = pho[phoReadPos1].middle;
		
		if(reader->selectVVD(pho[phoReadPos].vindex)==false)
		{
			fprintf(stderr,"select vvd %i failed\n",pho[phoReadPos].vindex);
			abort();
		}
		
		float index = pho[phoReadPos].vend*1000/reader->getFramePeriod();
		if(reader->getSegment(index,(void*)vvddata)==false)
		{
			fprintf(stderr,"read segment failed\n");
			abort();
		}
		
		if(reader->selectVVD(pho[phoReadPos1].vindex)==false)
		{
			fprintf(stderr,"select vvd %i failed\n",pho[phoReadPos].vindex);
			abort();
		}
		
		index = pho[phoReadPos1].vstart*1000/reader->getFramePeriod();
		if(reader->getSegment(index,(void*)vvddata2)==false)
		{
			fprintf(stderr,"read segment failed\n");
			abort();
		}
		
		int cepstrum_length = reader->getCepstrumLength();
		
		fprintf(stderr,"use delta for cat\n");
		for(int i=1;i<cepstrum_length+1;i++)
		{
			vvddata2[i]-=vvddata[i];
		}
		
		
		
		
		
		
	}
}
			
void VoiceSynth::drain(FILE* outfile)
{
	while(1)
	{
		
		int pitchReadPos1 = (pitchReadPos+1) % PITCH_RB_SIZE;
		int phoReadPos1 = (phoReadPos+1) % PHO_RB_SIZE;
		if(pitch[pitchReadPos1].pos==0) return;
		if(pho[phoReadPos1].start==0) return;
		if(currentTime*1000>pitch[pitchReadPos1].pos) {
			pitchReadPos = (pitchReadPos+1);
			pitchReadPos1 = (pitchReadPos+1) % PITCH_RB_SIZE;
		}
		if(currentTime*1000>pho[phoReadPos1].start)
		{
			phoReadPos = (phoReadPos+1) % PHO_RB_SIZE;
			
			static int counter=0;
			counter++;
			//fprintf(stderr,"next pho %i\n",counter);
			//fprintf(stderr,"vvdStart=%i vvdStart1=%i\n",pho[phoReadPos].start,pho[phoReadPos1].start);
			
		}
		
		if(currentTime==0)
		{
			//fprintf(stderr,"first pho\n");
			//fprintf(stderr,"vvdStart=%i vvdStart1=%i\n",pho[phoReadPos].start,pho[phoReadPos1].start);
			
			startCat();
			
			
			
		}
		
		if(pho[phoReadPos].vindex!=-1)
		{
		
			float x[3] = {0.001f*pho[phoReadPos].start, 0.001f*pho[phoReadPos].middle, 0.001f*pho[phoReadPos].end };
			float y[3] = {pho[phoReadPos].vstart      , pho[phoReadPos].vmiddle      , pho[phoReadPos].vend       };
			
			for(int i=0;i<33;i++)
			{
				vvddata[i]=0;
			}
		
			float index = interp_linear(x,y,3,currentTime)*1000/reader->getFramePeriod();
			//fprintf(stderr,"vvd: %i pos: %f\n",pho[phoReadPos].vindex,index);
			
			if(reader->selectVVD(pho[phoReadPos].vindex)==false)
			{
				fprintf(stderr,"select vvd %i failed\n",pho[phoReadPos].vindex);
				abort();
			}
			
			if(reader->getSegment((int)index,(void*)vvddata)==false)
			{
				fprintf(stderr,"read segment failed\n");
				abort();
			}
			
			
		}
		
		float ssinterp = 0.0;
		
		if(catStart>0 && currentTime*1000>=catStart)
		{
			//float factor=1.0;
			//if(currentTime*1000>=catMiddle) factor=-1.0;
			if(currentTime*1000<catMiddle)
			{
				ssinterp = -1.0*(catStart-currentTime*1000)/(catStart-catMiddle);
			}
			else if(currentTime*1000<catEnd)
			{
				ssinterp = 1.0-1.0*(catMiddle-currentTime*1000)/(catMiddle-catEnd);
			}
			else
			{
				//fprintf(stderr,"end of cat\n");
				catStart=0;
				startCat();
				
			}
			
			
		}
		
		int cepstrum_length = reader->getCepstrumLength();
		
		float* mel_cepstrum1 = &vvddata[1];
        float* mel_cepstrum2 = &vvddata[1+cepstrum_length];
        
        float* mel_cepstrum1_cat = &vvddata2[1];
        float* mel_cepstrum2_cat = &vvddata2[1+cepstrum_length];
        
        //this smoothing algorithm is also used in Vocaloid
        
        for(int i=0;i<cepstrum_length;i++)
        {
			mel_cepstrum1[i] += -0.5*ssinterp*mel_cepstrum1_cat[i];
			mel_cepstrum2[i] += -0.5*ssinterp*mel_cepstrum2_cat[i];
		}
		
		fprintf(stderr,"coeff: %f %f %f %f ssinterp: %f\n",mel_cepstrum1[0],mel_cepstrum1[1],
															mel_cepstrum1[2],mel_cepstrum1[3],ssinterp);
		
		

        MFCCDecompress(&spectrogram[rb],1,samplerate, fft_size,cepstrum_length,&mel_cepstrum1,false);
        MFCCDecompress(&aperiodicity[rb],1,samplerate, fft_size,cepstrum_length,&mel_cepstrum2,true);
		
		float myf0=0;
		
		if(pitch[pitchReadPos].hz==0){
			 myf0=0;
		}
		else
		{
			float left = pitch[pitchReadPos].hz;
			float right = pitch[pitchReadPos1].hz;
			
			if(right==0)
			{
				myf0 = left;
			}
			else
			{
				int len = pitch[pitchReadPos1].pos - pitch[pitchReadPos].pos;
				float a = 1.0*(pitch[pitchReadPos1].pos-1000.0*currentTime)/len; 
				myf0 = pitch[pitchReadPos1].hz*(1.0-a)+pitch[pitchReadPos].hz*a;
			}
		}
		//fprintf(stderr,"currentTime=%f f0=%f ssinterp=%f\n",currentTime,f0,ssinterp);
		
		f0[rb]=myf0;
		#if 0
		AddParameters(&f0[rb], 1, &spectrogram[rb], &aperiodicity[rb],&synth);
        #endif
        currentTime+=reader->getFramePeriod()/1000.0;
        rb = (rb+1) % number_of_pointers;

        float current_fill = synth.head_pointer - synth.current_pointer2;
        float fill_max = synth.number_of_pointers;
        float fill_ratio = current_fill/fill_max;

        if(fill_ratio>0.5) {
			fprintf(stderr,"world input buffer full\n");
			exit(1);
		};
		
		////
		#if 0
		int ret = SynthesisRealtime(&synth);
		
		if(ret)
		{
			fprintf(stderr,"MBROLOID returned audio data %f\n",fill_ratio);
			wavwrite_buffer(outfile,buffer_size,synth.buffer);
		}
        #endif

	}
}

void VoiceSynth::drain2(FILE* outfile)
{
    //FIXMe redundant code
#if 0	
	while(1)
	{
		int ret = SynthesisRealtime(&synth);
		
		fprintf(stderr,"drain2 %i\n",ret);
		
		if(ret)
		{
			float current_fill = synth.head_pointer - synth.current_pointer2;
			float fill_max = synth.number_of_pointers;
			float fill_ratio = current_fill/fill_max;
        
			fprintf(stderr,"MBROLOID returned audio data at EOF %f\n",fill_ratio);
			wavwrite_buffer(outfile,buffer_size,synth.buffer);
		}
		else
			return;
	}
#endif
}
