// Copyright 2012-2015 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//
// Test program for WORLD 0.1.2 (2012/08/19)
// Test program for WORLD 0.1.3 (2013/07/26)
// Test program for WORLD 0.1.4 (2014/04/29)
// Test program for WORLD 0.1.4_3 (2015/03/07)
// Test program for WORLD 0.2.0 (2015/05/29)
// Test program for WORLD 0.2.0_1 (2015/05/31)
// Test program for WORLD 0.2.0_2 (2015/06/06)
// Test program for WORLD 0.2.0_3 (2015/07/28)

// test.exe input.wav outout.wav f0 spec
// input.wav  : Input file
// output.wav : Output file
// f0         : F0 scaling (a positive number)
// spec       : Formant scaling (a positive number)

//this program is not working correctly, needs fix

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#include <conio.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#pragma warning(disable : 4996)
#endif
#if (defined (__linux__) || defined(__CYGWIN__) || defined(__APPLE__))
#include <stdint.h>
#include <sys/time.h>
#endif

#include "world/matlabfunctions.h"
#include "world/synthesis.h"  // This is the new function.
#include "world/cheaptrick.h"
#include "world/constantnumbers.h"

// Frame shift [msec]
#define FRAMEPERIOD 5.0

#if (defined (__linux__) || defined(__CYGWIN__) || defined(__APPLE__))
// POSIX porting section: implement timeGetTime() by gettimeofday(),
#ifndef DWORD
#define DWORD uint32_t
#endif
DWORD timeGetTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    DWORD ret = static_cast<DWORD>(tv.tv_usec / 1000 + tv.tv_sec * 1000);
    return ret;
}
#endif

#include "vvd.h"
#include "mfcc.h"
#include "epr.h"

const static double A4Frequency = 440.0;
const static double A4Note = 69.0;


//from https://github.com/haruneko/SimpleSynthesizer/blob/master/core/org/stand/util/MusicalNote.{h,cpp}

double frequencyFromNote(float note)
{
    if(note==0) return 0;
    return pow(2.0, (note - A4Note) / 12.0) * A4Frequency;
}



namespace
{



#define MAXRES 15





void DeCap(int fs, double *f0,
           int f0_length, double **spectrogram)
{
    int fft_size = GetFFTSizeForCheapTrick(fs);



    double* tmp = new double[fft_size/2+1];
    EprResonance res[MAXRES];
    for(int i=0; i<f0_length; i++)
    {

        EprSourceParams params;
        EprSourceEstimate(spectrogram[i],fft_size,fs,f0[i],&params);
        int count = EprVocalTractEstimate(spectrogram[i],fft_size,fs,f0[i],&params,tmp,res,MAXRES);
        printf("gaindb %f slope %f slopedepthdb %f\n",params.gaindb,params.slope,params.slopedepthdb);

        for(int j=0;j<fft_size/2+1;j++)
        {
            double f = j*fs/fft_size;
            double db = EprAtFrequency(&params,f,fs,res,count);
            spectrogram[i][j] = exp(db / TWENTY_OVER_LOG10);
        }
    }

}

void Decompress(double** spectrogram,double** aperiodicity,int f0_length,int fs, int fft_size,int cepstrum_length,
                float** mel_cepstrum1,float** mel_cepstrum2)
{
    DWORD elapsed_time;
    // Synthesis by the aperiodicity
    printf("\nDecompress\n");
    elapsed_time = timeGetTime();

    MFCCDecompress(spectrogram,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum1,false);
    MFCCDecompress(aperiodicity,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum2,true);


    printf("WORLD: %d [msec]\n", timeGetTime() - elapsed_time);
}



void WaveformSynthesis(double *f0, int f0_length, double **spectrogram,
                       double **aperiodicity, int fft_size, double frame_period, int fs,
                       int y_length, double *y)
{
    DWORD elapsed_time;
    // Synthesis by the aperiodicity
    printf("\nSynthesis\n");
    elapsed_time = timeGetTime();
    Synthesis(f0, f0_length, spectrogram, aperiodicity,
              fft_size, FRAMEPERIOD, fs, y_length, y);
    printf("WORLD: %d [msec]\n", timeGetTime() - elapsed_time);
}

}  // namespace

//-----------------------------------------------------------------------------
// Test program.
// test.exe input.wav outout.wav f0 spec flag
// input.wav  : argv[1] Input file
// output.wav : argv[2] Output file
// f0         : argv[3] F0 scaling (a positive number)
// spec       : argv[4] Formant shift (a positive number)
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    if (argc != 2 && argc != 3 && argc != 4 && argc != 5)
    {
        printf("usage: world_test infile.wav outfile.wav [pitch] [formant]\n");
        return 0;
    }

    FILE* f = fopen(argv[1],"rb");
    vvd_header hdr;
    fread(&hdr,1,sizeof(hdr),f);

    if(hdr.magic!=1430997325) return 0;
    //ignore version (0)
    int f0_length = hdr.f0_length;
    double frame_period = hdr.frame_period;
    int cepstrum_length = hdr.cepstrum_length;
    int fs = hdr.fs;
    int fft_size = GetFFTSizeForCheapTrick(fs);
    //ignore flags




    double *f0 = new double[f0_length];
    double *time_axis = new double[f0_length];
    float **mel_cepstrum1 = new float*[f0_length];
    float **mel_cepstrum2 = new float*[f0_length];

    double **spectrogram = new double *[f0_length];
    double **aperiodicity = new double *[f0_length];
    for (int i = 0; i < f0_length; ++i)
    {
        spectrogram[i] = new double[fft_size / 2 + 1];
        aperiodicity[i] = new double[fft_size / 2 + 1];
        mel_cepstrum1[i] = new float[cepstrum_length];
        mel_cepstrum2[i] = new float[cepstrum_length];
    }

    int chunksize = 1+cepstrum_length*2;
    float* chunk = new float[chunksize];

    for(int j=0; j<f0_length; j++)
    {
        fread(chunk,sizeof(float),chunksize,f);
        f0[j] = frequencyFromNote(chunk[0]);
        for(int i=0; i<cepstrum_length; i++)
        {
            mel_cepstrum1[j][i] = chunk[i+1];
            mel_cepstrum2[j][i] = chunk[i+1+cepstrum_length];
        }

    }

    Decompress(spectrogram,aperiodicity,f0_length,fs,fft_size,cepstrum_length,mel_cepstrum1,mel_cepstrum2);

    DeCap(fs, f0, f0_length, spectrogram);

    // The length of the output waveform
    int y_length =
        static_cast<int>((f0_length - 1) * FRAMEPERIOD / 1000.0 * fs) + 1;
    double *y = new double[y_length];
    // Synthesis
    WaveformSynthesis(f0, f0_length, spectrogram, aperiodicity, fft_size,
                      FRAMEPERIOD, fs, y_length, y);

    // Output
    wavwrite(y, y_length, fs, 16, argv[2]);

    printf("complete.\n");

    delete[] time_axis;
    delete[] f0;
    delete[] y;
    for (int i = 0; i < f0_length; ++i)
    {
        delete[] spectrogram[i];
        delete[] aperiodicity[i];
    }
    delete[] spectrogram;
    delete[] aperiodicity;

    return 0;
}
