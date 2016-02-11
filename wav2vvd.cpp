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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sekai.h"
#include "mfcc.h"
#include "note.h"

namespace {
  bool CheckLoadedFile(double *x, int fs, int nbit, int x_length) {
    if (x == NULL) {
      printf("error: File not found.\n");
      return false;
    }

    printf("File information\n");
    printf("Sampling : %d Hz %d Bit\n", fs, nbit);
    printf("Length %d [sample]\n", x_length);
    printf("Length %f [sec]\n", static_cast<double>(x_length) / fs);
    return true;
  }

  void F0Estimation(double *x, int x_length, int fs, int f0_length, double *f0,
		    double *time_axis) {
    double *refined_f0 = new double[f0_length];

    DioOption option;
    InitializeDioOption(&option);  // Initialize the option
    // Modification of the option
    option.frame_period = FRAMEPERIOD;
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

    printf("\nAnalysis\n");
    DWORD elapsed_time = timeGetTime();
    Dio(x, x_length, fs, &option, time_axis, f0);
    printf("DIO: %d [msec]\n", timeGetTime() - elapsed_time);

    for (int i = 0; i < f0_length; ++i)
    {
	if(isnan(f0[i])) printf("isnan %i\n",i);
    }
		

    // StoneMask is carried out to improve the estimation performance.
    elapsed_time = timeGetTime();
    StoneMask(x, x_length, fs, time_axis, f0, f0_length, refined_f0);
    printf("StoneMask: %d [msec]\n", timeGetTime() - elapsed_time);

    for (int i = 0; i < f0_length; ++i) f0[i] = refined_f0[i];

    //for (int i = 0; i < f0_length; ++i) if(f0[i])==) f0[i]=0;

    delete[] refined_f0;
    return;
  }

  void SpectralEnvelopeEstimation(double *x, int x_length, int fs,
				  double *time_axis, double *f0, int f0_length, double **spectrogram) {
    CheapTrickOption option;
    InitializeCheapTrickOption(&option);  // Initialize the option
    option.q1 = -0.15; // This value may be better one for HMM speech synthesis.

    DWORD elapsed_time = timeGetTime();
    CheapTrick(x, x_length, fs, time_axis, f0, f0_length, &option, spectrogram);
    printf("CheapTrick: %d [msec]\n", timeGetTime() - elapsed_time);

    int fft_size = GetFFTSizeForCheapTrick(fs,&option);

    for(int i=0;i<f0_length;i++)
      {
	for(int j=0;j<fft_size/2+1;j++)
	  {
	    if(isnan(spectrogram[i][j])) spectrogram[i][j]=world::kMySafeGuardMinimum;	
	  }
      }


  }

  void AperiodicityEstimation(double *x, int x_length, int fs, double *time_axis,
			      double *f0, int f0_length, int fft_size, double **aperiodicity) {
    D4COption option;
    InitializeD4COption(&option);  // Initialize the option

    DWORD elapsed_time = timeGetTime();
    // option is not implemented in this version. This is for future update.
    // We can use "NULL" as the argument.
    D4C(x, x_length, fs, time_axis, f0, f0_length, fft_size, &option,
        aperiodicity);
    printf("D4C: %d [msec]\n", timeGetTime() - elapsed_time);

  }

  void ParameterModification(int argc, char *argv[], int fs, double *f0,
			     int f0_length, double **spectrogram) {
					 
	CheapTrickOption option = {0};
    InitializeCheapTrickOption(&option);
    int fft_size = GetFFTSizeForCheapTrick(fs,&option);
    // F0 scaling
    if (argc >= 4) {
      double shift = atof(argv[3]);
      for (int i = 0; i < f0_length; ++i) f0[i] *= shift;
    }
    // Spectral stretching
    if (argc >= 5) {
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
	if (ratio < 1.0) {
	  for (int j = static_cast<int>(fft_size / 2.0 * ratio);
	       j <= fft_size / 2; ++j)
	    spectrogram[i][j] =
	      spectrogram[i][static_cast<int>(fft_size / 2.0 * ratio) - 1];
	}
      }
      delete[] spectrum1;
      delete[] spectrum2;
      delete[] freq_axis1;
      delete[] freq_axis2;
    }
  }

  void Compress(double** spectrogram,double** aperiodicity,int f0_length,int fs, int fft_size,int cepstrum_length,
		float** mel_cepstrum1,float** mel_cepstrum2) {
    DWORD elapsed_time;
    // Synthesis by the aperiodicity
    printf("\nCompress\n");
    elapsed_time = timeGetTime();
 
    MFCCCompress(spectrogram,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum1);
    MFCCCompress(aperiodicity,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum2);

    


  
  
    printf("COMPRESS: %d [msec]\n", timeGetTime() - elapsed_time);
  }

}  // namespace

//-----------------------------------------------------------------------------
// CheckHeader() checks the .wav header. This function can only support the
// monaural wave file. This function is only used in waveread().
//-----------------------------------------------------------------------------
bool CheckHeader(FILE *fp) {
  char data_check[5];
  fread(data_check, 1, 4, fp);  // "RIFF"
  data_check[4] = '\0';
  if (0 != strcmp(data_check, "RIFF")) {
    printf("RIFF error.\n");
    return false;
  }
  fseek(fp, 4, SEEK_CUR);
  fread(data_check, 1, 4, fp);  // "WAVE"
  if (0 != strcmp(data_check, "WAVE")) {
    printf("WAVE error.\n");
    return false;
  }
  fread(data_check, 1, 4, fp);  // "fmt "
  if (0 != strcmp(data_check, "fmt ")) {
    printf("fmt error.\n");
    return false;
  }
  fread(data_check, 1, 4, fp);  // 1 0 0 0
  if (!(16 == data_check[0] && 0 == data_check[1] &&
      0 == data_check[2] && 0 == data_check[3])) {
    printf("fmt (2) error.\n");
    return false;
  }
  fread(data_check, 1, 2, fp);  // 1 0
  if (!(1 == data_check[0] && 0 == data_check[1])) {
    printf("Format ID error.\n");
    return false;
  }
  fread(data_check, 1, 2, fp);  // 1 0
  if (!(1 == data_check[0] && 0 == data_check[1])) {
    printf("This function cannot support stereo file\n");
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
// GetParameters() extracts fp, nbit, wav_length from the .wav file
// This function is only used in wavread().
//-----------------------------------------------------------------------------
bool GetParameters(FILE *fp, int *fs, int *nbit, int *wav_length) {
  char data_check[5] = {0};
  data_check[4] = '\0';
  unsigned char for_int_number[4];
  fread(for_int_number, 1, 4, fp);
  *fs = 0;
  for (int i = 3; i >= 0; --i) *fs = *fs * 256 + for_int_number[i];
  // Quantization
  fseek(fp, 6, SEEK_CUR);
  fread(for_int_number, 1, 2, fp);
  *nbit = for_int_number[0];

  // Skip until "data" is found. 2011/03/28
  while (0 != fread(data_check, 1, 1, fp)) {
    if (data_check[0] == 'd') {
      fread(&data_check[1], 1, 3, fp);
      if (0 != strcmp(data_check, "data")) {
        fseek(fp, -3, SEEK_CUR);
      } else {
        break;
      }
    }
  }
  if (0 != strcmp(data_check, "data")) {
    printf("data error.\n");
    return false;
  }

  fread(for_int_number, 1, 4, fp);  // "data"
  *wav_length = 0;
  for (int i = 3; i >= 0; --i)
    *wav_length = *wav_length * 256 + for_int_number[i];
  *wav_length /= (*nbit / 8);
  return true;
}

double * wavread(char* filename, int *fs, int *nbit, int *wav_length) {
  FILE *fp = fopen(filename, "rb");
  if (NULL == fp) {
    printf("File not found.\n");
    return NULL;
  }

  if (CheckHeader(fp) == false) {
    fclose(fp);
    return NULL;
  }

  if (GetParameters(fp, fs, nbit, wav_length) == false) {
    fclose(fp);
    return NULL;
  }

  double *waveform = new double[*wav_length];
  if (waveform == NULL) return NULL;

  int quantization_byte = *nbit / 8;
  double zero_line = pow(2.0, *nbit - 1);
  double tmp, sign_bias;
  unsigned char for_int_number[4];
  for (int i = 0; i < *wav_length; ++i) {
    sign_bias = tmp = 0.0;
    fread(for_int_number, 1, quantization_byte, fp);  // "data"
    if (for_int_number[quantization_byte-1] >= 128) {
      sign_bias = pow(2.0, *nbit - 1);
      for_int_number[quantization_byte - 1] =
        for_int_number[quantization_byte - 1] & 0x7F;
    }
    for (int j = quantization_byte - 1; j >= 0; --j)
      tmp = tmp * 256.0 + for_int_number[j];
    waveform[i] = (tmp - sign_bias) / zero_line;
  }
  fclose(fp);
  return waveform;
}


//-----------------------------------------------------------------------------
// Test program.
// test.exe input.wav outout.wav f0 spec flag
// input.wav  : argv[1] Input file
// output.vvd : argv[2] Output file
//-----------------------------------------------------------------------------

#include "vvd.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("usage: world_test2 infile.wav outfile.wav\n");
    return 0;
  }
  int fs, nbit, x_length;
  double *x = wavread(argv[1], &fs, &nbit, &x_length);

  if (CheckLoadedFile(x, fs, nbit, x_length) == false) {
    printf("error: File not found.\n");
    return 0;
  }

  // Allocate memories
  // The number of samples for F0
  int f0_length = GetSamplesForDIO(fs, x_length, FRAMEPERIOD);
  double *f0 = new double[f0_length];
  double *time_axis = new double[f0_length];
  
  int cepstrum_length = 32;
  

  // FFT size for CheapTrick
  CheapTrickOption option = {0};
  InitializeCheapTrickOption(&option);
  int fft_size = GetFFTSizeForCheapTrick(fs,&option);
  
  double **spectrogram = new double *[f0_length];
  double **aperiodicity = new double *[f0_length];
  float **mel_cepstrum1 = new float*[f0_length];
  float **mel_cepstrum2 = new float*[f0_length];
  for (int i = 0; i < f0_length; ++i) {
    spectrogram[i] = new double[fft_size / 2 + 1];
    aperiodicity[i] = new double[fft_size / 2 + 1];
    mel_cepstrum1[i] = new float[cepstrum_length];
    mel_cepstrum2[i] = new float[cepstrum_length];    
  }

  // F0 estimation
  F0Estimation(x, x_length, fs, f0_length, f0, time_axis);

  // Spectral envelope estimation
  SpectralEnvelopeEstimation(x, x_length, fs, time_axis, f0, f0_length,
			     spectrogram);

  // Aperiodicity estimation by D4C
  AperiodicityEstimation(x, x_length, fs, time_axis, f0, f0_length,
			 fft_size, aperiodicity);
      
  Compress(spectrogram,aperiodicity,f0_length,fs,fft_size,cepstrum_length,mel_cepstrum1,mel_cepstrum2);
  
  FILE* f = fopen(argv[2],"wb");
  int chunksize = 1+cepstrum_length*2;
  float* chunk = new float[chunksize];
  
  
  
  vvd_header hdr;
  hdr.magic=1430997325;
  hdr.version=0;
  hdr.f0_length = f0_length;
  hdr.frame_period = FRAMEPERIOD;
  hdr.cepstrum_length = cepstrum_length;
  hdr.flags=0;
  hdr.fs=fs;
  
  fwrite(&hdr,1,sizeof(hdr),f);
  
  for(int j=0;j<f0_length;j++)
    {
      chunk[0] = noteFromFrequency(f0[j]);
      for(int i=0;i<cepstrum_length;i++)
	{
	  chunk[i+1] = mel_cepstrum1[j][i];
	  chunk[i+1+cepstrum_length] = mel_cepstrum2[j][i];
	}
      fwrite(chunk,sizeof(float),chunksize,f);
    }
  
  fclose(f);
  
       
 

  printf("complete.\n");

  delete[] x;
  delete[] time_axis;
  delete[] f0;

  for (int i = 0; i < f0_length; ++i) {
    delete[] spectrogram[i];
    delete[] aperiodicity[i];
    delete[] mel_cepstrum1[i];
    delete[] mel_cepstrum2[i];
  }
  delete[] spectrogram;
  delete[] aperiodicity;
  delete[] mel_cepstrum1;
  delete[] mel_cepstrum2;
  

  return 0;
}