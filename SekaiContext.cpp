#include "SekaiContext.h"
#include "mfcc.h"


void SekaiContext::Decompress() {
    DWORD elapsed_time;
    // Synthesis by the aperiodicity
    printf("\nDecompress\n");
    elapsed_time = timeGetTime();
  
    MFCCDecompress(spectrogram,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum1,false);
    MFCCDecompress(aperiodicity,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum2,true);
  
  
    printf("DECOMPRESS: %d [msec]\n", timeGetTime() - elapsed_time);
}

void SekaiContext::Compress() {
    DWORD elapsed_time;
    // Synthesis by the aperiodicity
    printf("\nCompress\n");
    elapsed_time = timeGetTime();
 
    MFCCCompress(spectrogram,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum1);
    MFCCCompress(aperiodicity,f0_length,fs, fft_size,cepstrum_length,mel_cepstrum2);

    printf("COMPRESS: %d [msec]\n", timeGetTime() - elapsed_time);
}
