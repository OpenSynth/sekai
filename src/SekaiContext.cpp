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
#include "sekai/mfcc.h"


void SekaiContext::Decompress() {
  DWORD elapsed_time;
  // Synthesis by the aperiodicity
  if(debug) printf("\nDecompress\n");
  elapsed_time = timeGetTime();
    
    
  MFCCDecompress(spectrogram,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum1,false);
  MFCCDecompress(aperiodicity,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum2,true);
  
  
  if(debug) printf("DECOMPRESS: %d [msec]\n", timeGetTime() - elapsed_time);
}

void SekaiContext::Compress() {
  DWORD elapsed_time;
  // Synthesis by the aperiodicity
    
  mel_cepstrum1 = new float*[f0_length];
  mel_cepstrum2 = new float*[f0_length];
  for(int i=0;i<f0_length;i++)
    {
      mel_cepstrum1[i]=new float[cepstrum_length];
      mel_cepstrum2[i]=new float[cepstrum_length];
    }
    
  if(debug) printf("\nCompress\n");
  elapsed_time = timeGetTime();
 
  MFCCCompress(spectrogram,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum1);
  MFCCCompress(aperiodicity,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum2);

  if(debug) printf("COMPRESS: %d [msec]\n", timeGetTime() - elapsed_time);
}



SekaiContext::~SekaiContext()
{
	for (int i = 0; i < f0_length; ++i) {
		if(mel_cepstrum1[i]) delete[] mel_cepstrum1[i];
		if(mel_cepstrum2[i]) delete[] mel_cepstrum2[i];
	}
	delete[] mel_cepstrum1;
	delete[] mel_cepstrum2;
	mel_cepstrum1 = 0;
	mel_cepstrum2 = 0;
}
