#include "sekai/WorldSynth2.h"

WorldSynth2::WorldSynth2(int bufferLen,int fft_size,int fs): OLABuffer(bufferLen), fft_size(fft_size), fs(fs){
    response = new double[fft_size];
    aperiodic_response = new double[fft_size];
    periodic_response = new double[fft_size];
    spectral_envelope = new double[fft_size/2+1];
    aperiodic_ratio = new double[fft_size/2+1];
#define ZERO_MEMORY(x) memset(&x,0,sizeof(x))
    ZERO_MEMORY(minimum_phase);
    InitializeMinimumPhaseAnalysis(fft_size, &minimum_phase);
    ZERO_MEMORY(inverse_real_fft);
    InitializeInverseRealFFT(fft_size, &inverse_real_fft);
    ZERO_MEMORY(forward_real_fft);
    InitializeForwardRealFFT(fft_size, &forward_real_fft);
    silence=true;
}

void WorldSynth2::doSynth()
{
    for(int i=0;i<fft_size;i++)
    {
        response[i]=0;
    }
    double current_vuv=f0;
    if(f0==0) f0=250;//default f0 if unvoiced
    float period = fs*1.0/f0;
    if(!silence)
    {
        int noise_size=period;
        GetOneFrameSegment2(
                    current_vuv,
                    noise_size,
                    fft_size,
                    fs,
                    &forward_real_fft,
                    &inverse_real_fft,
                    &minimum_phase,
                    response,
                    aperiodic_response,
                    periodic_response,
                    spectral_envelope,
                    aperiodic_ratio);
    }

    ola(response,fft_size,period);
}


