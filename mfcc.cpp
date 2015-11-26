/*
  Copyright 2011-2014	HAL(shuraba-P)
  Copyright      2014	Tobias Platen

  This file is part of Sekai.

  Sekai is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sekai is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Sekai.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include "world/fft.h"
#include "mfcc.h"

#include "world/cheaptrick.h"
#include "world/constantnumbers.h"

#include <stdio.h>

namespace {

  double getFrequency(double melScale);
  double getMelScale(double freq);
  void stretchToMelScale(double *melSpectrum, const double *spectrum, int spectrumLength, int maxFrequency);
  void stretchFromMelScale(double *spectrum, const double *melSpectrum, int spectrumLength, int maxFrequency);
  void linearStretch(double *dst, const double *src, double ratio, int length);
  double interpolateArray( double x, const double *p );


  void stretchToMelScale(double *melSpectrum, const double *spectrum, int spectrumLength, int maxFrequency)
  {
    double tmp = getMelScale(maxFrequency);
    for(int i = 0; i < spectrumLength; i++)
      {
        double dIndex = getFrequency((double)i / (double)spectrumLength * tmp) / (double)maxFrequency * (double)spectrumLength;
        if(dIndex <= spectrumLength-1.0){
	  melSpectrum[i] = interpolateArray(dIndex, spectrum);
        }else{
	  melSpectrum[i] = spectrum[spectrumLength - 1];
        }
      }
  }

  void stretchFromMelScale(double *spectrum, const double *melSpectrum, int spectrumLength, int maxFrequency)
  {
    double tmp = getMelScale(maxFrequency);
    for(int i = 0; i < spectrumLength; i++)
      {
        double dIndex = getMelScale((double)i / (double)spectrumLength * (double)maxFrequency) / tmp * (double)spectrumLength;
        double tmp=0;
        if(dIndex <= spectrumLength-1.0){
	  tmp = interpolateArray(dIndex, melSpectrum);
        }else{
	  tmp = melSpectrum[spectrumLength - 1];
        }
        spectrum[i] = tmp;
      }
  }

  void linearStretch(double *dst, const double *src, double ratio, int length)
  {
    for(int i = 0; i < length; i++)
      {
        double dIndex = (double)i * ratio;
        int iIndex = (int)dIndex;
        double r = dIndex - (double)iIndex;
        if(iIndex < 0 || iIndex >= length - 1) {
	  dst[i] = 0.0;
        } else {
	  dst[i] = pow(src[iIndex], 1.0 - r) * pow(src[iIndex], r);
        }
      }
  }

  double interpolateArray( double x, const double *p )
  {
    int t = (int)x;
    double r = x - (double)t;
    return ( p[t] * ( 1.0 - r ) + p[t+1] * r );
  }

  double  getMelScale(double freq){
    double ret = 1127.01048 * log(1 + freq / 700);
    return ret;
  }

  double  getFrequency(double melScale){
    double ret = 700 * (exp(melScale / 1127.01048) - 1);
    return ret;
  }

  void CalculateMelCepstrumOneFrameSegment(double* spectrogram,int fs, int fft_size,double *spectrum,fft_complex *cepstrum,fft_plan forward,fft_plan inverse,
					   int cepstrum_length,float* mel_cepstrum)
  {
    int j;
    stretchToMelScale(spectrum, spectrogram, fft_size / 2 + 1, fs / 2);


	




    for(j = 0; j <= fft_size / 2; j++) {
      if(spectrum[j]<=0) spectrum[j]=world::kMySafeGuardMinimum;	
      spectrum[j] = log(spectrum[j]) / fft_size;
    }
    for(; j < fft_size; j++) {
      spectrum[j] = spectrum[fft_size - j];
    }

    fft_execute(forward);

    //cepstral liftering
    for(j = 0; j < cepstrum_length; j++) {
      mel_cepstrum[j] = (float)cepstrum[j][0];
      cepstrum[j][1] = 0.0;
    }

    for(; j <= fft_size / 2; j++) {
      cepstrum[j][0] = cepstrum[j][1] = 0.0;
    }

    fft_execute(inverse);

    for(j = 0; j < fft_size; j++) {
      spectrum[j] = exp(spectrum[j]);
    }
    stretchFromMelScale(spectrogram, spectrum, fft_size / 2 + 1, fs / 2);
  }



  void CalculateSpectrumOneFrameSegment(double* spectrogram,int fs, int fft_size,double *spectrum,fft_complex *cepstrum,fft_plan inverse,
					int cepstrum_length,float* mel_cepstrum)
  {
    int j;
    //cepstral liftering
    for(j = 0; j < cepstrum_length; j++) {
      cepstrum[j][0] = (double)mel_cepstrum[j];
      cepstrum[j][1] = 0.0;
    }

    for(; j <= fft_size / 2; j++) {
      cepstrum[j][0] = cepstrum[j][1] = 0.0;
    }

    fft_execute(inverse);

    for(j = 0; j < fft_size; j++) {
      spectrum[j] = exp(spectrum[j]);

    }
    stretchFromMelScale(spectrogram, spectrum, fft_size / 2 + 1, fs / 2);
  }
}

void MFCCCompress(double** spectrogram,int f0_length,int fs, int fft_size,int cepstrum_length,float** mel_cepstrum)
{
  double *spectrum = new double[fft_size];
  fft_complex *cepstrum = new fft_complex[fft_size];
  fft_plan forward = fft_plan_dft_r2c_1d(fft_size, spectrum, cepstrum, FFT_ESTIMATE);
  fft_plan inverse = fft_plan_dft_c2r_1d(fft_size, cepstrum, spectrum, FFT_ESTIMATE);

  for(int i=0;i<f0_length;i++)
    {
      CalculateMelCepstrumOneFrameSegment(spectrogram[i],fs,fft_size,spectrum,cepstrum,forward,inverse,cepstrum_length,mel_cepstrum[i]);
    }

  delete[] spectrum;
  delete[] cepstrum;
  fft_destroy_plan(forward);
  fft_destroy_plan(inverse);

}

void MFCCDecompress(double** spectrogram,int f0_length,int fs, int fft_size,int cepstrum_length,float** mel_cepstrum,bool is_aperiodicity)
{
  double *spectrum = new double[fft_size];
  fft_complex *cepstrum = new fft_complex[fft_size];
  fft_plan inverse = fft_plan_dft_c2r_1d(fft_size, cepstrum, spectrum, FFT_ESTIMATE);

  for(int i=0;i<f0_length;i++)
    {
      CalculateSpectrumOneFrameSegment(spectrogram[i],fs,fft_size,spectrum,cepstrum,inverse,cepstrum_length,mel_cepstrum[i]);
      if(is_aperiodicity)
	{
	  double max=0;
	  for(int j=0;j<fft_size/2+1;j++)
	    {
	      double d = spectrogram[i][j];
	      if(d>=1) 
		{
		  if(max<d) max=d;
		}		
	    }
	  if(max>0) 
	    {
	      for(int j=0;j<fft_size/2+1;j++)
		{
		  spectrogram[i][j] /= max;
		  spectrogram[i][j] *= (1-world::kMySafeGuardMinimum);
		}
	    }}
    }

  delete[] spectrum;
  delete[] cepstrum;
  fft_destroy_plan(inverse);



}




