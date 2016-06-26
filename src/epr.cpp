/* Copyright 2014,2015 Tobias Platen */
/*  This file is part of Sekai.

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
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "epr.h"

#include <assert.h>

static int double_compare (const void* a,const void* b)
{
  double* x = (double*)b;
  double* y = (double*)a;
  if(*x==*y) return 0;
  if(*x>*y) return 1;
  else return -1;
}
void calc_slope(double* slope,double* sourcedb,double* f,double gaindb,double slopedepthdb,int count)
{
  for(int i=0;i<count;i++)
    {
      slope[i] = log ( (sourcedb[i]-gaindb) / slopedepthdb + 1 ) / f[i];
      if(isnan(slope[i]) && i>0) slope[i]=slope[i-1];
    }
  qsort((void*)slope,count,sizeof(double),double_compare);
}
void EprSourceEstimate(double* spectrogram,int fft_size,int fs,double f0,EprSourceParams* params)
{
  if(f0<50)
    {
      params->slope=0;
      params->gaindb=0;
      params->slopedepthdb=0;
      return;
    }
  double min = 0;
  int maxharm = fs/f0-2;
  double* sourcedb = new double[maxharm];
  double* fharm = new double[maxharm];
  double* slope = new double[maxharm];
  for(int i=1;i<fft_size/2-1;i++)
    {
      double current = TWENTY_OVER_LOG10 * log(spectrogram[i]);
      if(current<min) min=current;
    }
  for(int i=0;i<maxharm;i++)
    {
      double frq = (i+1)*f0;
      int bin = frq*fft_size/fs;
      sourcedb[i]=TWENTY_OVER_LOG10 * log(spectrogram[bin]);
      fharm[i]=frq;
    }
  double gaindb = sourcedb[0];
  double slopedepthdb = -min+gaindb;
  calc_slope(slope,sourcedb,fharm,gaindb,slopedepthdb,maxharm);
  params->gaindb=gaindb;
  params->slopedepthdb=slopedepthdb;

  int startindex = -1;

  for(int i=0;i<maxharm;i++)
    {
      //printf("slope[%i]: %f\n",i,slope[i]);
      if(startindex == -1 && slope[i]<0) startindex = i;
    }

  int slope_index = (maxharm-startindex)/2+startindex;
  params->slope=slope[slope_index];

  delete [] sourcedb;
  delete [] fharm;
  delete [] slope;
}


inline int sgn(double x)
{
  if (x==0)
    return 0;
  else
    return (x>0) ? 1 : -1;
}

int findMaxima(double *tmp,int fft_size,int fs,EprResonance* res, int nres)
{
  int sign=0;
  int oldsign=0;
  int i=0;
  for(int j=2;j<fft_size/2-8;j++)
    {
      sign = sgn(tmp[j]-tmp[j+1]);
      if(sign!=oldsign) {
	double a = tmp[j];
	double f = j*fs/fft_size;
	if(a>tmp[j-1] && a>tmp[j-2] && a>tmp[j+1] && a>tmp[j+2] && i < nres)
	  {
	    res[i].gain_db=tmp[j-1]; // select correct bin
	    res[i].f=f;
	    i++;
	  }
      }
    }
  return i;
}

void getBandwidth(double *tmp,int fft_size,int fs,EprResonance* res)
{
  double freq = res->f;
  double gain = res->gain_db;
  int bin = freq*fft_size/fs;
  //printf("bin %i\n",bin);
  double freq_left=0;
  double freq_right=0;
  for(int i=0;i<40;i++)
    {
      int index = i+bin;
      if(index>0 && index<fft_size/2-8)
        {
	  double freq2 = i*fs/fft_size;
	  double gaindrop = gain-tmp[index];
	  //printf("gaindrop R %f %f\n",freq2,gaindrop);
	  freq_right = freq2;
	  if(gaindrop>3) break;
        }
    }
  for(int i=0;i<40;i++)
    {
      int index = bin-i;
      if(index>0 && index<fft_size/2-8)
        {
	  double freq2 = i*fs/fft_size;
	  double gaindrop = gain-tmp[index];
	  //printf("gaindrop L %f %f\n",freq2,gaindrop);
	  if(gaindrop>3) break;
	  freq_left = freq2;
        }
    }

  double bw = sqrt(freq_left*freq_right);
  //FIXME one of both is zero: not found
  res->bw = bw;
  EprResonanceUpdate(res,fs);


}

int EprVocalTractEstimate(double* spectrogram,int fft_size,int fs,double f0,EprSourceParams* params,double* tmp,EprResonance* res, int nres)
{
  for(int i=0;i<fft_size/2;i++)
    {
      double f     = fs*i/fft_size;
      double spect = TWENTY_OVER_LOG10 * log(spectrogram[i]);
      double source  = params->gaindb + params->slopedepthdb * ( pow(M_E,params->slope*f)-1 );
      tmp[i] = spect-source;
    }
  int count = findMaxima(tmp,fft_size,fs,res,nres);
  for(int i=0;i<count;i++)
    {
      getBandwidth(tmp,fft_size,fs,&res[i]);
    }
  for(int i=0;i<count;i++)
    {
      EprResonanceUpdate(&res[i],fs);
    }
  for(int i=0;i<fft_size/2+1;i++)
    {
      double f = i*fs*1.0/fft_size;
      double db = TWENTY_OVER_LOG10 * log(spectrogram[i]);
      tmp[i] = db-EprAtFrequency(params,f,fs,res,nres);
    }
  return count;

}


double EprAtFrequency(EprSourceParams* params,double f,int fs,EprResonance* res,int n_res)
{
  double source  = 0;
  if(params) source = params->gaindb + params->slopedepthdb * ( pow(M_E,params->slope*f)-1 );

  double accu = 0.0000001;//TODO: use minimal value here
  for(int i=0;i<n_res;i++)
    {
      double x = EprResonanceAtFrequency(&res[i],f,fs);
      assert(!isnan(x));
      accu += x;
    }

  return (TWENTY_OVER_LOG10 * log(accu)) + source;
}

double EprResonanceAtFrequency(EprResonance* me,double f,int fs)
{
  if(me->a==0) return 0.0;
  //klatt filter second order
  double w = 2*M_PI*f/(double)fs;
  double real = 1 - me->b * cos(w) - me->c * cos(2*w);
  double imag = 0 - me->b * sin(w) - me->c * sin(2*w);
  return me->a / sqrt(real*real+imag*imag) / me->norm;
  //we only need to know the absolute value of our complex number
}

void EprResonanceUpdate(EprResonance* me,int fs) {
  if(me->gain_db==0)
    {
      me->a = 0;
      me->b = 0;
      me->c = 0;
      return;
    }
  double tmp = exp(-M_PI*me->bw/(double)fs);
  me->c = -tmp*tmp;
  me->b = 2.0*tmp*cos(2*M_PI*me->f/(double)fs);
  me->a = 1.0 - me->b - me->c;
  me->norm = 1;
  me->norm = EprResonanceAtFrequency(me,me->f,fs);
  me->a *= exp(me->gain_db/TWENTY_OVER_LOG10);
}

