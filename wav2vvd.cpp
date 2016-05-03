#include "SekaiContext.h"
#include "world/audioio.h"
#include "world/cheaptrick.h"
#include "midi.h"
#include "vvd.h"

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

  int x_length;
  int fs;
  double *x = wavReadMono(argv[1],&fs,&x_length);
  //check for errors
  
  SekaiContext ctx;
  // You must set fs and frame_period before analysis/synthesis.
  ctx.fs = fs;
  ctx.DisplayInformation(x_length);
  ctx.cepstrum_length = 32;

  // 5.0 ms is the default value.
  // Generally, the inverse of the lowest F0 of speech is the best.
  // However, the more elapsed time is required.
  ctx.frame_period = 5.0;

  // F0 estimation
  ctx.F0Estimation(x, x_length);

  // Spectral envelope estimation
  ctx.SpectralEnvelopeEstimation(x, x_length);

  // Aperiodicity estimation by D4C
  ctx.AperiodicityEstimation(x, x_length);
  
  //allocate memory for compress
  ctx.mel_cepstrum1 = new float*[ctx.f0_length];
  ctx.mel_cepstrum2 = new float*[ctx.f0_length];
  for (int i = 0; i < ctx.f0_length; ++i) {
    ctx.mel_cepstrum1[i] = new float[ctx.cepstrum_length];
    ctx.mel_cepstrum2[i] = new float[ctx.cepstrum_length];
  }

  ctx.Compress();
  
  FILE* f = fopen(argv[2],"wb");
  int chunksize = 1+ctx.cepstrum_length*2;
  float* chunk = new float[chunksize];
  
  vvd_header hdr;
  hdr.magic=1430997325;
  hdr.version=0;
  hdr.f0_length = ctx.f0_length;
  hdr.frame_period = ctx.frame_period;
  hdr.cepstrum_length = ctx.cepstrum_length;
  hdr.flags=0;
  hdr.fs=fs;
  
  fwrite(&hdr,1,sizeof(hdr),f);
  
  for(int j=0;j<ctx.f0_length;j++)
    {
      chunk[0] = noteFromFrequency(ctx.f0[j]);
      for(int i=0;i<ctx.cepstrum_length;i++)
	{
	  chunk[i+1] = ctx.mel_cepstrum1[j][i];
	  chunk[i+1+ctx.cepstrum_length] = ctx.mel_cepstrum2[j][i];
	}
      fwrite(chunk,sizeof(float),chunksize,f);
    }
  
  fclose(f);
  
  delete[] chunk;     
 

  printf("complete.\n");
  
  delete[] x;

 
  return 0;
}
