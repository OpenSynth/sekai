//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
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
// Test program for WORLD 0.2.0_4 (2015/11/15)
// Test program for WORLD in GitHub (2015/11/16-)
// Test program for WORLD in NotABug
// Latest update: see git log

// test.exe input.wav outout.wav f0 spec
// input.wav  : Input file
// output.wav : Output file
// f0         : F0 scaling (a positive number)
// spec       : Formant scaling (a positive number)
//
// 2016/04/19:
// Note: This version output three speech synthesized by different algorithms.
//       When the filename is "output.wav", "01output.wav", "02output.wav" and
//       "03output.wav" are generated. They are almost all the same.
//-----------------------------------------------------------------------------

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

// For .wav input/output functions.
#include "audioio.h"

// WORLD core functions.
// Note: win.sln uses an option in Additional Include Directories.
// To compile the program, the option "-I $(SolutionDir)..\src" was set.
#include "world/d4c.h"
#include "world/dio.h"
#include "world/matlabfunctions.h"
#include "world/cheaptrick.h"
#include "world/stonemask.h"
#include "world/synthesis.h"
#include "world/synthesisrealtime.h"

#include "sekai.h"

#if (defined (__linux__) || defined(__CYGWIN__) || defined(__APPLE__))
// Linux porting section: implement timeGetTime() by gettimeofday(),
#ifndef DWORD
#define DWORD uint32_t
#endif
DWORD timeGetTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  DWORD ret = static_cast<DWORD>(tv.tv_usec / 1000 + tv.tv_sec * 1000);
  return ret;
}
#endif

//-----------------------------------------------------------------------------
// struct for WORLD
// This struct is an option.
// Users are NOT forced to use this struct.
//-----------------------------------------------------------------------------
typedef struct {
  double frame_period;
  int fs;

  double *f0;
  double *time_axis;
  int f0_length;

  double **spectrogram;
  double **aperiodicity;
  float** mel_cepstrum1;
  float** mel_cepstrum2;
  
  
  int fft_size;
  int cepstrum_length;
} WorldParameters;

namespace {

void DisplayInformation(int fs, int x_length) {
  printf("File information\n");
  printf("Sampling rate: %d Hz\n", fs);
  printf("Length %d [sample]\n", x_length);
  printf("Length %f [sec]\n", static_cast<double>(x_length) / fs);
}

void F0Estimation(double *x, int x_length, WorldParameters *world_parameters) {
  DioOption option = {0};
  InitializeDioOption(&option);

  // Modification of the option
  // When you You must set the same value.
  // If a different value is used, you may suffer a fatal error because of a
  // illegal memory access.
  option.frame_period = world_parameters->frame_period;

  // Valuable option.speed represents the ratio for downsampling.
  // The signal is downsampled to fs / speed Hz.
  // If you want to obtain the accurate result, speed should be set to 1.
  option.speed = 1;

  // You should not set option.f0_floor to under world::kFloorF0.
  // If you want to analyze such low F0 speech, please change world::kFloorF0.
  // Processing speed may sacrify, provided that the FFT length changes.
  option.f0_floor = 71.0;

  // You can give a positive real number as the threshold.
  // Most strict value is 0, but almost all results are counted as unvoiced.
  // The value from 0.02 to 0.2 would be reasonable.
  option.allowed_range = 0.1;

  // Parameters setting and memory allocation.
  world_parameters->f0_length = GetSamplesForDIO(world_parameters->fs,
    x_length, world_parameters->frame_period);
  world_parameters->f0 = new double[world_parameters->f0_length];
  world_parameters->time_axis = new double[world_parameters->f0_length];
  double *refined_f0 = new double[world_parameters->f0_length];

  printf("\nAnalysis\n");
  DWORD elapsed_time = timeGetTime();
  Dio(x, x_length, world_parameters->fs, &option, world_parameters->time_axis,
      world_parameters->f0);
  printf("DIO: %d [msec]\n", timeGetTime() - elapsed_time);

  // StoneMask is carried out to improve the estimation performance.
  elapsed_time = timeGetTime();
  StoneMask(x, x_length, world_parameters->fs, world_parameters->time_axis,
      world_parameters->f0, world_parameters->f0_length, refined_f0);
  printf("StoneMask: %d [msec]\n", timeGetTime() - elapsed_time);

  for (int i = 0; i < world_parameters->f0_length; ++i)
    world_parameters->f0[i] = refined_f0[i];

  delete[] refined_f0;
  return;
}

void SpectralEnvelopeEstimation(double *x, int x_length,
    WorldParameters *world_parameters) {
  CheapTrickOption option = {0};
  InitializeCheapTrickOption(&option);

  // This value may be better one for HMM speech synthesis.
  // Default value is -0.09.
  option.q1 = -0.15;

  // Important notice (2016/02/02)
  // You can control a parameter used for the lowest F0 in speech.
  // You must not set the f0_floor to 0.
  // It will cause a fatal error because fft_size indicates the infinity.
  // You must not change the f0_floor after memory allocation.
  // You should check the fft_size before excucing the analysis/synthesis.
  // The default value (71.0) is strongly recommended.
  // On the other hand, setting the lowest F0 of speech is a good choice
  // to reduce the fft_size.
  option.f0_floor = 71.0;

  // Parameters setting and memory allocation.
  world_parameters->fft_size =
    GetFFTSizeForCheapTrick(world_parameters->fs, &option);
  world_parameters->spectrogram = new double *[world_parameters->f0_length];
  for (int i = 0; i < world_parameters->f0_length; ++i) {
    world_parameters->spectrogram[i] =
      new double[world_parameters->fft_size / 2 + 1];
  }

  DWORD elapsed_time = timeGetTime();
  CheapTrick(x, x_length, world_parameters->fs, world_parameters->time_axis,
      world_parameters->f0, world_parameters->f0_length, &option,
      world_parameters->spectrogram);
  printf("CheapTrick: %d [msec]\n", timeGetTime() - elapsed_time);
}

void AperiodicityEstimation(double *x, int x_length,
    WorldParameters *world_parameters) {
  D4COption option = {0};
  InitializeD4COption(&option);

  // Parameters setting and memory allocation.
  world_parameters->aperiodicity = new double *[world_parameters->f0_length];
  for (int i = 0; i < world_parameters->f0_length; ++i) {
    world_parameters->aperiodicity[i] =
      new double[world_parameters->fft_size / 2 + 1];
  }

  DWORD elapsed_time = timeGetTime();
  // option is not implemented in this version. This is for future update.
  // We can use "NULL" as the argument.
  D4C(x, x_length, world_parameters->fs, world_parameters->time_axis,
      world_parameters->f0, world_parameters->f0_length,
      world_parameters->fft_size, &option, world_parameters->aperiodicity);
  printf("D4C: %d [msec]\n", timeGetTime() - elapsed_time);
}

void ParameterModification(int argc, char *argv[], int fs, int f0_length,
    int fft_size, double *f0, double **spectrogram) {
  // F0 scaling
  if (argc >= 4) {
    double shift = atof(argv[3]);
    for (int i = 0; i < f0_length; ++i) f0[i] *= shift;
  }
  if (argc < 5) return;

  // Spectral stretching
  double ratio = atof(argv[4]);
  double *freq_axis1 = new double[fft_size];
  double *freq_axis2 = new double[fft_size];
  double *spectrum1 = new double[fft_size];
  double *spectrum2 = new double[fft_size];

  for (int i = 0; i <= fft_size / 2; ++i) {
    freq_axis1[i] = ratio * i / fft_size * fs;
    freq_axis2[i] = static_cast<double>(i) / fft_size * fs;
  }
  for (int i = 0; i < f0_length; ++i) {
    for (int j = 0; j <= fft_size / 2; ++j)
      spectrum1[j] = log(spectrogram[i][j]);
    interp1(freq_axis1, spectrum1, fft_size / 2 + 1, freq_axis2,
      fft_size / 2 + 1, spectrum2);
    for (int j = 0; j <= fft_size / 2; ++j)
      spectrogram[i][j] = exp(spectrum2[j]);
    if (ratio >= 1.0) continue;
    for (int j = static_cast<int>(fft_size / 2.0 * ratio);
        j <= fft_size / 2; ++j)
      spectrogram[i][j] =
      spectrogram[i][static_cast<int>(fft_size / 2.0 * ratio) - 1];
  }
  delete[] spectrum1;
  delete[] spectrum2;
  delete[] freq_axis1;
  delete[] freq_axis2;
}

void Decompress(double** spectrogram,double** aperiodicity,int f0_length,int fs, int fft_size,int cepstrum_length,
		  float** mel_cepstrum1,float** mel_cepstrum2) {
    DWORD elapsed_time;
    // Synthesis by the aperiodicity
    printf("\nDecompress\n");
    elapsed_time = timeGetTime();
  
    MFCCDecompress(spectrogram,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum1,false);
    MFCCDecompress(aperiodicity,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum2,true);
  
  
    printf("X: %d [msec]\n", timeGetTime() - elapsed_time);
}


void WaveformSynthesis(WorldParameters *world_parameters, int fs,
    int y_length, double *y) {
  DWORD elapsed_time;
  // Synthesis by the aperiodicity
  printf("\nSynthesis 1 (conventional algorithm)\n");
  elapsed_time = timeGetTime();
  Synthesis(world_parameters->f0, world_parameters->f0_length,
      world_parameters->spectrogram, world_parameters->aperiodicity,
      world_parameters->fft_size, world_parameters->frame_period, fs,
      y_length, y);
  printf("WORLD: %d [msec]\n", timeGetTime() - elapsed_time);
}

void WaveformSynthesis2(WorldParameters *world_parameters, int fs,
  int y_length, double *y) {
  DWORD elapsed_time;
  printf("\nSynthesis 2 (All frames are added at the same time)\n");
  elapsed_time = timeGetTime();

  WorldSynthesizer synthesizer = { 0 };
  int buffer_size = 64;
  InitializeSynthesizer(world_parameters->fs, world_parameters->frame_period,
      world_parameters->fft_size, buffer_size, 1, &synthesizer);

  // All parameters are added at the same time.
  AddParameters(world_parameters->f0, world_parameters->f0_length,
      world_parameters->spectrogram, world_parameters->aperiodicity,
      &synthesizer);

  int index;
  for (int i = 0; Synthesis2(&synthesizer) != 0; ++i) {
    index = i * buffer_size;
    for (int j = 0; j < buffer_size; ++j) {
      y[j + index] = synthesizer.buffer[j];
    }
  }

  printf("WORLD: %d [msec]\n", timeGetTime() - elapsed_time);
  DestroySynthesizer(&synthesizer);
}

void WaveformSynthesis3(WorldParameters *world_parameters, int fs,
  int y_length, double *y) {
  DWORD elapsed_time;
  // Synthesis by the aperiodicity
  printf("\nSynthesis 3 (Ring buffer is efficiently used.)\n");
  elapsed_time = timeGetTime();

  WorldSynthesizer synthesizer = { 0 };
  int buffer_size = 64;
  InitializeSynthesizer(world_parameters->fs, world_parameters->frame_period,
    world_parameters->fft_size, buffer_size, 20, &synthesizer);

  int offset = 0;
  int index = 0;
  for (int i = 0; i < world_parameters->f0_length;) {
    // Add one frame (i shows the frame index that should be added)
    if (AddParameters(&world_parameters->f0[i], 1,
      &world_parameters->spectrogram[i], &world_parameters->aperiodicity[i],
      &synthesizer) == 1) ++i;

    // Synthesize speech with length of buffer_size sample.
    // It is repeated until the function returns 0
    // (it suggests that the synthesizer cannot generate speech).
    while (Synthesis2(&synthesizer) != 0) {
      index = offset * buffer_size;
      for (int j = 0; j < buffer_size; ++j)
        y[j + index] = synthesizer.buffer[j];
      offset++;
    }

    // Check the "Lock" (Please see synthesisrealtime.h)
    if (IsLocked(&synthesizer) == 1) {
      printf("Locked!\n");
      break;
    }
  }

  printf("WORLD: %d [msec]\n", timeGetTime() - elapsed_time);
  DestroySynthesizer(&synthesizer);
}

void DestroyMemory(WorldParameters *world_parameters) {
  delete[] world_parameters->time_axis;
  delete[] world_parameters->f0;
  for (int i = 0; i < world_parameters->f0_length; ++i) {
    delete[] world_parameters->spectrogram[i];
    delete[] world_parameters->aperiodicity[i];
  }
  delete[] world_parameters->spectrogram;
  delete[] world_parameters->aperiodicity;
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
int main(int argc, char *argv[]) {
  if (argc != 2 && argc != 3 && argc != 4 && argc != 5) {
    printf("usage: world_test input.wav output.wav [formant] [time]\n");
    return -2;
  }

  //---------------------------------------------------------------------------
  // Analysis part
  //---------------------------------------------------------------------------
  // 2016/02/02
  // A new struct is introduced to implement safe program.
  WorldParameters world_parameters = { 0 };
  // You must set fs and frame_period before analysis/synthesis.
  FILE* f = fopen(argv[1],"rb");
  vvd_header hdr;
  fread(&hdr,1,sizeof(hdr),f);
  
  if(hdr.magic!=1430997325) return 0;
  //ignore version (0)
  world_parameters.f0_length = hdr.f0_length;
  world_parameters.frame_period = hdr.frame_period;
  world_parameters.cepstrum_length = hdr.cepstrum_length;
  world_parameters.fs = hdr.fs;
  CheapTrickOption option = {0};
  InitializeCheapTrickOption(&option);
  world_parameters.fft_size = GetFFTSizeForCheapTrick(world_parameters.fs,&option);
  //ignore flags
  
  fprintf(stderr,"f0_length %i\n",world_parameters.f0_length);
  
  world_parameters.f0 = new double[world_parameters.f0_length];
  world_parameters.time_axis = new double[world_parameters.f0_length];
  world_parameters.mel_cepstrum1 = new float*[world_parameters.f0_length];
  world_parameters.mel_cepstrum2 = new float*[world_parameters.f0_length];
  
  world_parameters.spectrogram = new double *[world_parameters.f0_length];
  world_parameters.aperiodicity = new double *[world_parameters.f0_length];
  for (int i = 0; i < world_parameters.f0_length; ++i) {
    world_parameters.spectrogram[i] = new double[world_parameters.fft_size / 2 + 1];
    world_parameters.aperiodicity[i] = new double[world_parameters.fft_size / 2 + 1];
    world_parameters.mel_cepstrum1[i] = new float[world_parameters.cepstrum_length];
    world_parameters.mel_cepstrum2[i] = new float[world_parameters.cepstrum_length];
  }
  
  int chunksize = 1+world_parameters.cepstrum_length*2;
  float* chunk = new float[chunksize];
  
  for(int j=0;j<world_parameters.f0_length;j++)
    {
      fread(chunk,sizeof(float),chunksize,f);  
      world_parameters.f0[j] = frequencyFromNote(chunk[0]);
      for(int i=0;i<world_parameters.cepstrum_length;i++)
	{
	  world_parameters.mel_cepstrum1[j][i] = chunk[i+1];
	  world_parameters.mel_cepstrum2[j][i] = chunk[i+1+world_parameters.cepstrum_length];
	}
	
    }
    
  Decompress(world_parameters.spectrogram,world_parameters.aperiodicity,world_parameters.f0_length,world_parameters.fs,
	world_parameters.fft_size,world_parameters.cepstrum_length,world_parameters.mel_cepstrum1,world_parameters.mel_cepstrum2);
  
  ParameterModification(argc, argv, world_parameters.fs, world_parameters.f0_length,
    world_parameters.fft_size, world_parameters.f0,
    world_parameters.spectrogram);

  //---------------------------------------------------------------------------
  // Synthesis part (2016/04/19)
  // There are three samples in speech synthesis
  // 1: Conventional synthesis
  // 2: Example of real-time synthesis
  // 3: Example of real-time synthesis (Ring buffer is efficiently used)
  //---------------------------------------------------------------------------
  char filename[100];
  // The length of the output waveform
  int y_length = static_cast<int>((world_parameters.f0_length - 1) *
    world_parameters.frame_period / 1000.0 * world_parameters.fs) + 1;
  double *y = new double[y_length];

  // Synthesis 1 (conventional synthesis)
  for (int i = 0; i < y_length; ++i) y[i] = 0.0;
  WaveformSynthesis(&world_parameters, world_parameters.fs, y_length, y);
  sprintf(filename, "01%s", argv[2]);
  wavWriteMono(filename,world_parameters.fs,y_length,y);

  // Synthesis 2 (All frames are added at the same time)
  for (int i = 0; i < y_length; ++i) y[i] = 0.0;
  WaveformSynthesis2(&world_parameters, world_parameters.fs, y_length, y);
  sprintf(filename, "02%s", argv[2]);
  wavWriteMono(filename,world_parameters.fs,y_length,y);

  // Synthesis 3 (Ring buffer is efficiently used.)
  for (int i = 0; i < y_length; ++i) y[i] = 0.0;
  WaveformSynthesis3(&world_parameters, world_parameters.fs, y_length, y);
  sprintf(filename, "03%s", argv[2]);
  wavWriteMono(filename,world_parameters.fs,y_length,y);

  delete[] y;
  DestroyMemory(&world_parameters);

  printf("complete.\n");
  return 0;
}
