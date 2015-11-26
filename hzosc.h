#include <fftw3.h>

#define N_OSC 100
#define FFT_SIZE 2048

class IFFTOsc
{
public:
  double phase[N_OSC];
  double amp[N_OSC];
  double frq[N_OSC];
    IFFTOsc ();
  void processOneFrame ();
  double output_buffer[FFT_SIZE/2];
  int fs;
private:
  float window[FFT_SIZE];
  fftw_complex *spectrum;
  double *waveform;
  fftw_plan inverse_c2r;
  double rover[FFT_SIZE/2];
};

