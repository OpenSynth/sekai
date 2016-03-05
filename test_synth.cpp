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

#ifdef USE_EPR
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
#endif



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

//move to common.h

float segment_interp(float x,float a,float b,float factor)
{
	assert(x>=a);
	assert(b>=x);
	float right = b-x;
    return factor*(x-a)/(b-a);
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

class MiraiSynth
{

	float _mid;
	float _left;
	float _right;
	bool _concat;
	bool _lastConcat;
	int _currentIndex;
	double _currentPos;
	////////////////////////////////////////////////////

	MinimumPhaseAnalysis _minimum_phase;
	ForwardRealFFT _forward_real_fft;
	InverseRealFFT _inverse_real_fft;
	float* _vvdData;
	int _fs;
	int _fftSize;
	double* _spectrogram;
	double* _aperiodicity;  
	double* _rightSpectrogram;
	double* _rightAperiodicity;    
	double* _periodicResponse;
	double* _aperiodicResponse;
	double* _response;
	double* _olaFrame;
	int _cepstrumLength;
	float _vvdPitch;
	VVDReader* _vvd;

	//input seg
	int _seg_count;
	segment_t _seg[100];
	
	//output buffer
	double _signal_length; 
	int _signal_length_samples;
	float* _signal;

	#ifdef USE_EPR
	EpRFrame _currentEpR;
	EpRFrame _leftEpR;
	EpRFrame _rightEpR;
	#endif

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



public:
	int init()
	{
		//init vvd_reader

		_vvd = new VVDReader();
		_vvd->addVVD("vvd/0.vvd");
		_vvd->addVVD("vvd/1.vvd");
		_vvd->addVVD("vvd/2.vvd");

		_vvdData = new float[_vvd->getFrameSize()/sizeof(float)];
		_cepstrumLength = _vvd->getCepstrumLength();
		_fs =_vvd->getSamplerate();
		CheapTrickOption option;
		InitializeCheapTrickOption(&option);
		_fftSize = GetFFTSizeForCheapTrick(_fs,&option);
		_spectrogram = new double[_fftSize/2+1];
		_aperiodicity = new double[_fftSize/2+1];
		_rightSpectrogram = new double[_fftSize/2+1];
		_rightAperiodicity = new double[_fftSize/2+1];
		InitializeInverseRealFFT(_fftSize, &_inverse_real_fft);
		InitializeForwardRealFFT(_fftSize, &_forward_real_fft);
		InitializeMinimumPhaseAnalysis(_fftSize, &_minimum_phase);
		_periodicResponse = new double[_fftSize];
		_aperiodicResponse = new double[_fftSize];
		_response = new double[_fftSize];
		
		#ifdef USE_EPR
		EpRFrameAllocate(&_currentEpR,20,_fftSize);
		EpRFrameAllocate(&_leftEpR,20,_fftSize);
		EpRFrameAllocate(&_rightEpR,20,_fftSize);
		#endif
	}
	
	int prepareSynth()
	{
		//setup test segment list

		_seg_count = 3;

		_seg[0].x[0]=0.0;  _seg[0].y[0]=0.2;
		_seg[0].x[1]=1.3;	_seg[0].y[1]=0.8;
		_seg[0].n_points = 2;
		_seg[0].r=0.2;
		_seg[0].vvd_index=0;

		_seg[1].x[0]=1.3;  _seg[1].y[0]=0.2;
		_seg[1].x[1]=2.7;	_seg[1].y[1]=0.8;
		_seg[1].n_points = 2;
		_seg[1].l=0.1;
		_seg[1].r=0.2;
		_seg[1].vvd_index=1;

		_seg[2].x[0]=2.7;	_seg[2].y[0]=0.2;
		_seg[2].x[1]=3.9;	_seg[2].y[1]=0.8;
		_seg[2].n_points = 2;
		_seg[2].l=0.2;
		_seg[2].vvd_index=2;
		
		_signal_length = X_END(_seg[_seg_count-1]);
		_signal_length_samples = _fs*_signal_length+2*_fftSize;
		_signal = new float[_signal_length_samples];

		update_concat_zone(_seg);
	}
	
	int runSynth()
	{
		
			printf("currentPos %f\n",_currentPos);
			if(_currentIndex>_seg_count-1)
			{
				printf("END OF FILE\n");
				return 0;
			}
			float f0=200;
			double y_pos = interp_linear(_seg[_currentIndex].x,_seg[_currentIndex].y,_seg[_currentIndex].n_points,_currentPos);
			int idx= _seg[_currentIndex].vvd_index;
			_vvd->selectVVD(idx);

			float framePeriod = _vvd->getFramePeriod();
			/////////
			float fractionalIndex=y_pos*1000.0/framePeriod;

			if(_vvd->getSegment(fractionalIndex,_vvdData)==false)
			{
				printf("PANIC: cannot read from VVD\n");
				exit(1);
			}

			uncompressVVDFrame(_spectrogram,_aperiodicity);
        
			if(_currentPos>=_left && _concat==false && _lastConcat==false)
			{
				printf("START CONCAT %f %f %f\n",_left,_mid,_right);

				double y_left  = interp_linear(_seg[_currentIndex].x,_seg[_currentIndex].y,_seg[_currentIndex].n_points,_mid);
				double y_right = interp_linear(_seg[_currentIndex+1].x,_seg[_currentIndex+1].y,_seg[_currentIndex+1].n_points,_mid);
				float fractionalIndex_left=y_left*1000.0/framePeriod;
				float fractionalIndex_right=y_right*1000.0/framePeriod;
            
				_vvd->selectVVD(_seg[_currentIndex].vvd_index);
				if(_vvd->getSegment(fractionalIndex_left,_vvdData)==false)
				{
					printf("PANIC: cannot read from VVD [L]\n");
					exit(1);
				}
			
				#ifdef USE_EPR
				uncompressVVDFrame(_TODOspectrogram,_TODOaperiodicity);
				EpREstimate(&_leftEpR,_spectrogram,_fftSize,_fs,f0);
				#endif
			
				_vvd->selectVVD(_seg[_currentIndex+1].vvd_index);
				if(_vvd->getSegment(fractionalIndex_right,_vvdData)==false)
				{
					printf("PANIC: cannot read from VVD [R]\n");
					exit(1);
				}
				uncompressVVDFrame(_rightSpectrogram,_rightAperiodicity);
				#ifdef USE_EPR
					EpREstimate(&_rightEpR,_spectrogram,_fftSize,_fs,f0);
				#endif
            
				_concat=true;
			}
			#ifndef BETA
			if(_concat)
			{
				if(_currentPos<_mid)
				{
					double factor;
					factor = segment_interp(_currentPos,_left,_mid,1.0);
					printf("%f\n",factor);
					for(int i=0;i<_fftSize/2+1;i++)
					{
						_spectrogram[i] = _spectrogram[i] * (1-factor) + _rightSpectrogram[i] * factor;
						_aperiodicity[i] = _aperiodicity[i] * (1-factor) + _rightAperiodicity[i] * factor;
					}
				}
			}
			#endif
			#ifdef USE_EPR
			if(_concat)
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
		#endif

        if(_concat==true && _currentPos>=_right)
        {
            _concat=false;
            printf("END CONCAT %i\n",_currentIndex);
            if(_currentIndex<_seg_count-1)
                update_concat_zone(&_seg[_currentIndex]);
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
            assert(index<_signal_length_samples);
				_signal[index]+=_response[i]*0.5;
        }
        //TODO: synthesis
        double period = 1.0/f0;
        _currentPos+=period; //period

        double currentEnd=X_END(_seg[_currentIndex]);
        if(_currentPos>currentEnd) {
            _currentIndex++;
        }
        

	printf("END step\n");
	return 1;
    //}

    //write signal to sndfile

    


}
void writeWavFile()
{
	SF_INFO info = {0};
    info.samplerate = _fs;
    info.channels = 1;
    info.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
    SNDFILE* sf=sf_open("test_synth.wav",SFM_WRITE,&info);
    sf_write_float(sf,_signal,_signal_length_samples);
    sf_close(sf);
}
};

int main()
{
	MiraiSynth synth;
	//alloc
	//synthN
	synth.init();
	synth.prepareSynth();
	while(synth.runSynth());
	synth.writeWavFile();
}
