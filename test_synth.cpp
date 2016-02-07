#include "VVDReader.h"
#include "mfcc.h"
#include "midi.h"

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <world/cheaptrick.h>
#include <world/common.h>
#include <world/constantnumbers.h>
#include <world/matlabfunctions.h>
#include <world/synthesis.h>

#include <sndfile.h>


#define printf(...)  {}

#define MAXPOINTS 8

typedef struct {
	float x[MAXPOINTS];
	float y[MAXPOINTS];
	float l;
	float r;
	int n_points;
	int vvd_index;
} segment_t;

#define X_END(p) p.x[p.n_points-1]

float segment_interp(float x,float a,float b,float factor)
{
	return factor*(a-x)/(a-b);
}

//timebase for concatenation of segments
float _mid;
float _left;
float _right;
bool _concat=false;
bool _lastConcat=false;
int _currentIndex=0;
double _currentPos=0;
////////////////////////////////////////////////////

MinimumPhaseAnalysis _minimum_phase = {0};
ForwardRealFFT _forward_real_fft = {0};
InverseRealFFT _inverse_real_fft = {0};
float* _vvdData=NULL;
int _fs=44100;
int _fftSize=0;
double* _spectrogram;
double* _aperiodicity;   
double* _periodicResponse;
double* _aperiodicResponse;
double* _response;
double* _olaFrame;
int _cepstrumLength;
float _vvdPitch;

void uncompressVVDFrame(double* spectrogram,double* aperiodicity)
{
		 float* tmp=_vvdData;
         _vvdPitch = *tmp;
         float* cepstrum1 = tmp+1;
         float* cepstrum2 = cepstrum1+_cepstrumLength;
         
         //assert(frequencyFromNote(vvdPitch)<400);
         //printf("uncompress %f\n",frequencyFromNote(vvdPitch));

         MFCCDecompress(&spectrogram,1,_fs,_fftSize,_cepstrumLength,&cepstrum1,false);
         MFCCDecompress(&aperiodicity,1,_fs,_fftSize,_cepstrumLength,&cepstrum2,true);
}


void update_concat_zone(segment_t* s)
{
	assert(X_END(s[0])==s[1].x[0]);
	_mid=X_END(s[0]);
	assert(X_END(s[0])-s[0].r>s[0].x[0]);
	_left=X_END(s[0])-s[0].r;
	assert(s[1].x[0]+s[1].l<X_END(s[1]));
	_right=s[1].x[0]+s[1].l;
}

float interp_linear(float* x,float* y,int nx,float ref)
{
    int i;
    for(i=0; i<nx-1; i++)
    {
        if(ref>=x[i] && ref <= x[i+1])
        {
            float x1=x[i];
            float x2=x[i+1];
            float tmp = (ref-x1)/(x2-x1);
            return y[i]*(1-tmp)+y[i+1]*tmp;
        }
    }
    fprintf(stderr,"INTERP_LINEAR: out of range\n");
    return NAN;
}

int main()
{
	//init vvd_reader

	VVDReader* vvd = new VVDReader();
	vvd->addVVD("vvd/0.vvd");
	vvd->addVVD("vvd/1.vvd");
	vvd->addVVD("vvd/2.vvd");
	
	_vvdData = new float[vvd->getFrameSize()/sizeof(float)];
	_cepstrumLength = vvd->getCepstrumLength();
	_fs =vvd->getSamplerate();
	CheapTrickOption option;
	InitializeCheapTrickOption(&option);
    _fftSize = GetFFTSizeForCheapTrick(_fs,&option);
    printf("fft_size %i\n",_fftSize);
   	_spectrogram = new double[_fftSize/2+1];
	_aperiodicity = new double[_fftSize/2+1];
	InitializeInverseRealFFT(_fftSize, &_inverse_real_fft);
	InitializeForwardRealFFT(_fftSize, &_forward_real_fft);
	InitializeMinimumPhaseAnalysis(_fftSize, &_minimum_phase);
    _periodicResponse = new double[_fftSize];
    _aperiodicResponse = new double[_fftSize];
    _response = new double[_fftSize];
	
	//setup test segment list

	int s_count=3;
	segment_t s[s_count];

	s[0].x[0]=0.0;  s[0].y[0]=0.2;
	s[0].x[1]=1.0;	s[0].y[1]=0.8;
	s[0].n_points = 2;
	s[0].r=0.2;
	s[0].vvd_index=0;

	s[1].x[0]=1.0;  s[1].y[0]=0.2;
	s[1].x[1]=2.0;	s[1].y[1]=0.8;
	s[1].n_points = 2;
	s[1].l=0.1;
	s[1].r=0.2;
	s[1].vvd_index=1;

	s[2].x[0]=2.0;	s[2].y[0]=0.2;
	s[2].x[1]=3.0;	s[2].y[1]=0.8;
	s[2].n_points = 2;
	s[2].l=0.2;
	s[2].vvd_index=2;
	
	double signal_length = X_END(s[s_count-1]);
	int signal_length_samples = _fs*signal_length+2*_fftSize;
	double* signal = new double[signal_length_samples];
	
	int _lastIDX=-1;
	
	update_concat_zone(s);

	SNDFILE* debugfile;
	{
	SF_INFO info = {0};
	info.samplerate = _fs;
	info.channels = 1;
	info.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
	debugfile=sf_open("debug_synth.wav",SFM_WRITE,&info);
	}
	
	while(1)
	{
		if(_currentIndex>s_count-1) 
		{
			printf("END OF FILE\n");
			break; 
		}
		
		double y_pos = interp_linear(s[_currentIndex].x,s[_currentIndex].y,s[_currentIndex].n_points,_currentPos);
		printf("VVD: %i at %f\n",s[_currentIndex].vvd_index,y_pos);
		
		//get impulse response
		
		int idx= s[_currentIndex].vvd_index;
		
		if(idx!=_lastIDX) {
			vvd->selectVVD(idx);
			printf("segment changed %i\n",idx);
			_lastIDX=idx;
		}
		float framePeriod = vvd->getFramePeriod();
		float fractionalIndex=y_pos*1000.0/framePeriod;
		printf("fractIndex: %f\n",fractionalIndex);
		
		if(vvd->getSegment(fractionalIndex,_vvdData)==false)
		{
			printf("PANIC: cannot read from VVD\n");
			exit(1);
		}
		
		uncompressVVDFrame(_spectrogram,_aperiodicity);
		
		if(_vvdPitch>400)
		{
		printf("invalid VVD %f\n",_vvdPitch);
		for(int i=0;i<_fftSize/2+1;i++)
		{
			printf("VVD: %i %f %f\n",i,_spectrogram[i],_aperiodicity[i]);
		}
		}
		
		
		float f0=200;
		int noise_size = ((float)_fs)/f0;
		printf("noise_size %i\n",noise_size);
		double current_vuv=1.0; //only vowels
		
		
	SynthesisOneImpulseResponse(
	_fftSize,
	noise_size,
	current_vuv,
	_spectrogram,
	_aperiodicity,
	&_forward_real_fft,
    &_inverse_real_fft,
    &_minimum_phase,
    _periodicResponse,
    _aperiodicResponse,
    _response);
    
    sf_write_double(debugfile,_response,_fftSize);
    
    int samplePos = _currentPos*_fs;
    printf("sample pos %i\n",samplePos);
    for(int i=0;i<_fftSize;i++)
    {
		int index = i+samplePos;
		assert(index<signal_length_samples);
		signal[index]+=_response[i];
	}
		
		
		//TODO: synthesis
		double period = 1.0/f0;
		_currentPos+=period; //period
		
		
		if(_currentPos>=_left && _concat==false && _lastConcat==false)
		{
			 printf("START CONCAT %f %f %f\n",_left,_mid,_right);
			 
			 double y_left  = interp_linear(s[_currentIndex].x,s[_currentIndex].y,s[_currentIndex].n_points,_mid);
			 double y_right = interp_linear(s[_currentIndex+1].x,s[_currentIndex+1].y,s[_currentIndex+1].n_points,_mid);
			 
			 printf("MAPPED SEGMENTS %f %f\n",y_left,y_right);
			 
			 _concat=true;
		}

		if(_concat && _currentPos<_mid)
			printf("L %f: %f ss_interp=%f\n",_currentPos,segment_interp(_currentPos,_left,_mid,-0.5),segment_interp(_currentPos,_left,_mid,1.0));
		if(_concat && _currentPos>=_mid)
			printf("R %f: %f\n",_currentPos,segment_interp(_currentPos,_right,_mid,+0.5));

		if(_concat==true && _currentPos>=_right)
		{
			_concat=false;
			printf("END CONCAT %i\n",_currentIndex);
			if(_currentIndex<s_count-1)
				update_concat_zone(&s[_currentIndex]);
			else
				_lastConcat=true;
		}
		
		double currentEnd=X_END(s[_currentIndex]);
		if(_currentPos>currentEnd) {
			_currentIndex++;
		}
					
	}
	
	//write signal to sndfile

	SF_INFO info = {0};
	info.samplerate = _fs;
	info.channels = 1;
	info.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
	SNDFILE* sf=sf_open("test_synth.wav",SFM_WRITE,&info);
	sf_write_double(sf,signal,signal_length_samples);
	sf_close(sf);
	
	sf_close(debugfile);


}
