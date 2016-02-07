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

#include "common.h"
#include "epr.h"

typedef struct {
	EprSourceParams src;
	EprResonance* res;
	int n_res;
	double* residual;
} EpRFrame;

void EpRFrameAllocate(EpRFrame* frame,int n_res, int fft_size)
{
	frame->n_res=n_res;
	frame->res=new EprResonance[n_res];
	frame->residual=new double[fft_size/2+1];
}

void EpREstimate(EpRFrame* frame,double* spectrogram,int fft_size,int fs,double f0)
{
	EprSourceEstimate(spectrogram,fft_size,fs,f0,&frame->src);
	EprVocalTractEstimate(spectrogram,fft_size,fs,f0,&frame->src,frame->residual,frame->res, frame->n_res);
}

int EpRFindIndex(double f,EpRFrame* frame,int max)
{
	double min=22*1000;
	int index=-1;
	for(int i=0;i<20;i++)
	{
		double delta = fabs(f-frame->res[i].f);
		if(delta<min)
		{
			min=delta;
			index=i;
		}
	}
	if(min>max) index=-1;
	//printf("MIN: %f INDEX: %i\n",min,index);
	return index;
}



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

///////////
EpRFrame _currentEpR;
EpRFrame _leftEpR;
EpRFrame _rightEpR;

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
    _spectrogram = new double[_fftSize/2+1];
    _aperiodicity = new double[_fftSize/2+1];
    InitializeInverseRealFFT(_fftSize, &_inverse_real_fft);
    InitializeForwardRealFFT(_fftSize, &_forward_real_fft);
    InitializeMinimumPhaseAnalysis(_fftSize, &_minimum_phase);
    _periodicResponse = new double[_fftSize];
    _aperiodicResponse = new double[_fftSize];
    _response = new double[_fftSize];
    
    EpRFrameAllocate(&_currentEpR,20,_fftSize);
    EpRFrameAllocate(&_leftEpR,20,_fftSize);
    EpRFrameAllocate(&_rightEpR,20,_fftSize);

    //setup test segment list

    int s_count=3;
    segment_t s[s_count];

    s[0].x[0]=0.0;  s[0].y[0]=0.2;
    s[0].x[1]=1.3;	s[0].y[1]=0.8;
    s[0].n_points = 2;
    s[0].r=0.2;
    s[0].vvd_index=0;

    s[1].x[0]=1.3;  s[1].y[0]=0.2;
    s[1].x[1]=2.7;	s[1].y[1]=0.8;
    s[1].n_points = 2;
    s[1].l=0.1;
    s[1].r=0.2;
    s[1].vvd_index=1;

    s[2].x[0]=2.7;	s[2].y[0]=0.2;
    s[2].x[1]=3.9;	s[2].y[1]=0.8;
    s[2].n_points = 2;
    s[2].l=0.2;
    s[2].vvd_index=2;

    double signal_length = X_END(s[s_count-1]);
    int signal_length_samples = _fs*signal_length+2*_fftSize;
    double* signal = new double[signal_length_samples];

    update_concat_zone(s);

    while(1)
    {
        if(_currentIndex>s_count-1)
        {
            printf("END OF FILE\n");
            break;
        }
        float f0=200;

        double y_pos = interp_linear(s[_currentIndex].x,s[_currentIndex].y,s[_currentIndex].n_points,_currentPos);

        int idx= s[_currentIndex].vvd_index;
        vvd->selectVVD(idx);

        float framePeriod = vvd->getFramePeriod();
        /////////
        float fractionalIndex=y_pos*1000.0/framePeriod;

        if(vvd->getSegment(fractionalIndex,_vvdData)==false)
        {
            printf("PANIC: cannot read from VVD\n");
            exit(1);
        }

        uncompressVVDFrame(_spectrogram,_aperiodicity);
        
        if(_currentPos>=_left && _concat==false && _lastConcat==false)
        {
            printf("START CONCAT %f %f %f\n",_left,_mid,_right);

            double y_left  = interp_linear(s[_currentIndex].x,s[_currentIndex].y,s[_currentIndex].n_points,_mid);
            double y_right = interp_linear(s[_currentIndex+1].x,s[_currentIndex+1].y,s[_currentIndex+1].n_points,_mid);
            float fractionalIndex_left=y_left*1000.0/framePeriod;
            float fractionalIndex_right=y_right*1000.0/framePeriod;
            
			vvd->selectVVD(s[_currentIndex].vvd_index);
            if(vvd->getSegment(fractionalIndex_left,_vvdData)==false)
			{
				printf("PANIC: cannot read from VVD [L]\n");
				exit(1);
			}
			uncompressVVDFrame(_spectrogram,_aperiodicity);
			EpREstimate(&_leftEpR,_spectrogram,_fftSize,_fs,f0);
			
			vvd->selectVVD(s[_currentIndex+1].vvd_index);
			if(vvd->getSegment(fractionalIndex_right,_vvdData)==false)
			{
				printf("PANIC: cannot read from VVD [R]\n");
				exit(1);
			}
			uncompressVVDFrame(_spectrogram,_aperiodicity);
			EpREstimate(&_rightEpR,_spectrogram,_fftSize,_fs,f0);
			
			///printf("L SRC: %f %f %f\n",_leftEpR.src.gaindb,_leftEpR.src.slope,_leftEpR.src.slopedepthdb);
			///printf("R SRC: %f %f %f\n",_rightEpR.src.gaindb,_rightEpR.src.slope,_rightEpR.src.slopedepthdb);
			
			for(int i=0;i<20;i++)
			{
					//printf("RES[%i]: %f %f %f | %f %f %f \n",i,_leftEpR.res[i].f,_leftEpR.res[i].bw,_leftEpR.res[i].gain_db,
					//										   _rightEpR.res[i].f,_rightEpR.res[i].bw,_rightEpR.res[i].gain_db);
			}
			//printf("-----------------------------\n");
            
            _concat=true;
        }
        
        if(1) if(_concat)
        {
				EpREstimate(&_currentEpR,_spectrogram,_fftSize,_fs,f0);
				
				double factor;
				char side;
				if(_currentPos<_mid)
				{
					side='L';
					factor = segment_interp(_currentPos,_left,_mid,-0.5);
				}
				else
				{
					side='R';
					factor = segment_interp(_currentPos,_right,_mid,+0.5);
				}
					
				#define CORR(X) _currentEpR.src.X+=factor*(_leftEpR.src.X-_rightEpR.src.X);
				CORR(slope)
				CORR(slopedepthdb)
				CORR(gaindb)
				//printf("C SRC: %f %f %f [%c]\n",_currentEpR.src.gaindb,_currentEpR.src.slope,_currentEpR.src.slopedepthdb,side);
				#undef CORR
					
				
				for(int i=0;i<20;i++)
				{
					if(_currentEpR.res[i].f>0){
						 int l = EpRFindIndex(_currentEpR.res[i].f,&_leftEpR,75);
						 int r = EpRFindIndex(_currentEpR.res[i].f,&_rightEpR,75);
						 double lfrq=0;
						 double rfrq=0;
						 if(l>=0) lfrq = _leftEpR.res[l].f;
						 if(r>=0) rfrq = _rightEpR.res[r].f;
						 
						 
						 if(l>-1 && r>-1)
						 {
							 //printf("EpR change: %f %f\n",_currentEpR.res[i].f,factor);
							 #define CORR(X) _currentEpR.res[i].X+=factor*(_leftEpR.res[l].X-_rightEpR.res[r].X);
							 CORR(gain_db);
							 CORR(f);
							 CORR(bw);
							 #undef CORR
							 //printf("RES[%i]: %f %f %f [L=%i(%f) R=%i(%f)] CORR\n",i,_currentEpR.res[i].f,_currentEpR.res[i].bw,_currentEpR.res[i].gain_db,l,lfrq,r,rfrq);
							 EprResonanceUpdate(&_currentEpR.res[i],_fs);
						 } 
						 
						 
					 }
				}
				
				
				for(int i=0;i<_fftSize/2+1;i++)
				{
					double frq = i*1.0*_fs/_fftSize;
					double dB = EprAtFrequency(&_currentEpR.src,frq,_fs,_currentEpR.res, _currentEpR.n_res);
					//dB-=10;
					dB+=_currentEpR.residual[i];
					_spectrogram[i] = exp(dB/TWENTY_OVER_LOG10);
				}
				
		}

        if(_concat==true && _currentPos>=_right)
        {
            _concat=false;
            printf("END CONCAT %i\n",_currentIndex);
            if(_currentIndex<s_count-1)
                update_concat_zone(&s[_currentIndex]);
            else
                _lastConcat=true;
        }

        
        int noise_size = ((float)_fs)/f0;
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

        int samplePos = _currentPos*_fs;
        for(int i=0;i<_fftSize;i++)
        {
            int index = i+samplePos;
            assert(index<signal_length_samples);
				signal[index]+=_response[i]*0.5;
        }
        
       


        //TODO: synthesis
        double period = 1.0/f0;
        _currentPos+=period; //period


        

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


}
