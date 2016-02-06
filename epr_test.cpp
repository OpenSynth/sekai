#include "epr.h"
#include <stdio.h>
#include <math.h>
#include "common.h"


float init_f[6] =       {915.597,1408.48,1892.801,4025.529,4509.241,6000};
float init_bw[6]      = {174.803,167.995,125.245 ,411.338 ,429.417, 50};
float init_gain_db[6] = {20,20,10,7,5,3};
float init_controls[3] = { 22.5, -7E-5, 77  };



   

int main()
{
	int fs=44100;
	EprResonance res[6];

	for(int i=0;i<6;i++)
    	{
	    res[i].f = init_f[i];
	    res[i].bw = init_bw[i];
    	    res[i].gain_db = init_gain_db[i];	
    	    EprResonanceUpdate(&res[i],fs);
    	}

	EprSourceParams src;
	
    	src.gaindb = init_controls[0];
    	src.slope = init_controls[1];
    	src.slopedepthdb = init_controls[2];

	printf("SRC: %f %f %f\n",src.gaindb,src.slope,src.slopedepthdb);

	for(int i=0;i<6;i++)
    	{
	    printf("RES[%i]: %f %f %f\n",i,res[i].f,res[i].bw,res[i].gain_db);
	}


	int fft_size = 1024*2;

	double* spectrogram = new double[fft_size/2+1];

	double f0=300;

	for(int i=0;i<fft_size/2+1;i++)
	{
		double f = i*fs*1.0/fft_size;
		spectrogram[i] = exp(EprAtFrequency(&src,f,fs,res,6)/TWENTY_OVER_LOG10);
	}

	EprSourceEstimate(spectrogram,fft_size,fs,f0,&src);

	printf("SRC: %f %f %f\n",src.gaindb,src.slope,src.slopedepthdb);

	double* residual = new double[fft_size/2+1];

	EprVocalTractEstimate(spectrogram,fft_size,fs,f0,&src,residual,res,6);

	for(int i=0;i<6;i++)
    	{
	    printf("RES[%i]: %f %f %f\n",i,res[i].f,res[i].bw,res[i].gain_db);
	}

	for(int i=0;i<fft_size/2+1;i++)
	{
		double f = i*fs*1.0/fft_size;
		double db = TWENTY_OVER_LOG10 * log(spectrogram[i]);
		printf("FRQ: %f %f\n",f,db-EprAtFrequency(&src,f,fs,res,6));
	}

	
	


	
}
