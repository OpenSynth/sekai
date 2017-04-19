#ifndef WORLDSYNTH2_H
#define WORLDSYNTH2_H
//move to sekai
#include "sekai/OLABuffer.h"
#include <world/synthesis.h>
class WorldSynth2 : public OLABuffer
{
    float f0;
    bool silence;
    int fft_size;
    int fs;
    ForwardRealFFT forward_real_fft;
    InverseRealFFT inverse_real_fft;
    MinimumPhaseAnalysis minimum_phase;

    double *aperiodic_response;
    double *periodic_response;
    double *response;
    double *spectral_envelope;
    double *aperiodic_ratio;
    
public:
    WorldSynth2(int bufferLen,int fft_size,int fs);
    void setF0(float f0){this->f0= f0;}
    void setSilence(){this->silence= true;}
    void doSynth();
};
#endif
