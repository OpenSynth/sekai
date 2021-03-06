/*
	Sekai - addons for the WORLD speech toolkit
    Copyright (C) 2016 Tobias Platen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 

#include "sekai/SekaiContext.h"
#include "sekai/vvd.h"
#include "world/cheaptrick.h"
#include "sekai/midi.h"
#include <sndfile.h>

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

 
  SekaiContext ctx;
  
  FILE* f = fopen(argv[1],"rb");
  vvd_header hdr;
  fread(&hdr,1,sizeof(hdr),f);
  
  if(hdr.magic!=1430997325) return 0;
  //ignore version (0)
  ctx.f0_length = hdr.f0_length;
  ctx.frame_period = hdr.frame_period;
  ctx.cepstrum_length = hdr.cepstrum_length;
  ctx.fs = hdr.fs;
  CheapTrickOption option = {0};
  InitializeCheapTrickOption(0,&option);
  ctx.fft_size = GetFFTSizeForCheapTrick(ctx.fs,&option);
  //ignore flags
  
  fprintf(stderr,"f0_length %i\n",ctx.f0_length);
  
  ctx.f0 = new double[ctx.f0_length];
  ctx.time_axis = new double[ctx.f0_length];
  ctx.mel_cepstrum1 = new float*[ctx.f0_length];
  ctx.mel_cepstrum2 = new float*[ctx.f0_length];
  
  ctx.spectrogram = new double *[ctx.f0_length];
  ctx.aperiodicity = new double *[ctx.f0_length];
  for (int i = 0; i < ctx.f0_length; ++i) {
    ctx.spectrogram[i] = new double[ctx.fft_size / 2 + 1];
    ctx.aperiodicity[i] = new double[ctx.fft_size / 2 + 1];
    ctx.mel_cepstrum1[i] = new float[ctx.cepstrum_length];
    ctx.mel_cepstrum2[i] = new float[ctx.cepstrum_length];
  }
  
  int chunksize = 1+ctx.cepstrum_length*2;
  float* chunk = new float[chunksize];
  
  for(int j=0;j<ctx.f0_length;j++)
    {
      fread(chunk,sizeof(float),chunksize,f);  
      ctx.f0[j] = frequencyFromNote(chunk[0]);
      for(int i=0;i<ctx.cepstrum_length;i++)
	{
	  ctx.mel_cepstrum1[j][i] = chunk[i+1];
	  ctx.mel_cepstrum2[j][i] = chunk[i+1+ctx.cepstrum_length];
	}
	
    }
    
  ctx.Decompress();
  
  ctx.ParameterModification(argc, argv);

  //---------------------------------------------------------------------------
  // Synthesis part (2016/04/19)
  // There are three samples in speech synthesis
  // 1: Conventional synthesis
  // 2,3 jack is used instead
  // The length of the output waveform
  int y_length = ctx.Len();
  double *y = new double[y_length];

  // Synthesis 1 (conventional synthesis)
  for (int i = 0; i < y_length; ++i) y[i] = 0.0;
  ctx.WaveformSynthesis( hdr.fs, y_length, y);
  
  SF_INFO info;
  memset(&info,0,sizeof(info));
  info.format =  SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  info.samplerate = hdr.fs;
  info.channels = 1;
  SNDFILE* sf = sf_open(argv[2],SFM_WRITE,&info);
  int count = sf_write_double(sf,y,y_length);
  sf_close(sf);

  delete[] y;
  //TODO: proper memory management DestroyMemory(&world_parameters);
 

  printf("complete.\n");
  return 0;
}
