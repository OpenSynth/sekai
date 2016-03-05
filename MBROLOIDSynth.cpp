#include "VVDReader.h"
#include "mfcc.h"
#include "midi.h"

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <world/cheaptrick.h>
#include <world/common.h>
#include <world/constantnumbers.h>
#include <world/matlabfunctions.h>
#include <world/synthesis.h>

#include <sndfile.h>

#include "common.h"
#include "epr.h"
#include "obOlaBuffer.h"
#include "MBROLOIDSynth.h"


float segment_interp(float x, float a, float b, float factor)
{
    assert (x >= a);
    assert (b >= x);
    float right = b - x;
    return factor * (x - a) / (b - a);
}

float interp_linear(float *x, float *y, int nx, float ref)
{
    int i;
    for (i = 0; i < nx - 1; i++)
    {
        if (ref >= x[i] && ref <= x[i + 1])
        {
            float x1 = x[i];
            float x2 = x[i + 1];
            float tmp = (ref - x1) / (x2 - x1);
            return y[i] * (1 - tmp) + y[i + 1] * tmp;
        }
    }
    fprintf (stderr, "INTERP_LINEAR: out of range\n");
    return NAN;
}

void MBROLOIDSynth::uncompressVVDFrame(double *spectrogram, double *aperiodicity)
{
    float *tmp = _vvdData;
    _vvdPitch = *tmp;
    float *cepstrum1 = tmp + 1;
    float *cepstrum2 = cepstrum1 + _cepstrumLength;

    MFCCDecompress (&spectrogram, 1, _fs, _fftSize, _cepstrumLength, &cepstrum1,false);
    MFCCDecompress (&aperiodicity, 1, _fs, _fftSize, _cepstrumLength,&cepstrum2, true);
}


void MBROLOIDSynth::update_concat_zone (segment_t * s)
{
    assert (X_END (s[0]) == s[1].x[0]);
    _mid = X_END (s[0]);
    assert (X_END (s[0]) - s[0].r > s[0].x[0]);
    _left = X_END (s[0]) - s[0].r;
    assert (s[1].x[0] + s[1].l < X_END (s[1]));
    _right = s[1].x[0] + s[1].l;
}

int MBROLOIDSynth::init ()
{
    //init vvd_reader

    _vvd = new VVDReader ();
    _vvd->addVVD ("vvd/0.vvd");
    _vvd->addVVD ("vvd/1.vvd");
    _vvd->addVVD ("vvd/2.vvd");

    _vvdData = new float[_vvd->getFrameSize () / sizeof (float)];
    _cepstrumLength = _vvd->getCepstrumLength ();
    _fs = _vvd->getSamplerate ();
    CheapTrickOption option;
    InitializeCheapTrickOption (&option);
    _fftSize = GetFFTSizeForCheapTrick (_fs, &option);
    _spectrogram = new double[_fftSize / 2 + 1];
    _aperiodicity = new double[_fftSize / 2 + 1];
    _rightSpectrogram = new double[_fftSize / 2 + 1];
    _rightAperiodicity = new double[_fftSize / 2 + 1];
    InitializeInverseRealFFT (_fftSize, &_inverse_real_fft);
    InitializeForwardRealFFT (_fftSize, &_forward_real_fft);
    InitializeMinimumPhaseAnalysis (_fftSize, &_minimum_phase);
    _periodicResponse = new double[_fftSize];
    _aperiodicResponse = new double[_fftSize];
    _response = new double[_fftSize];
}

int MBROLOIDSynth::prepareSynth ()
{
    //setup test segment list

    _seg_count = 3;

    _seg[0].x[0] = 0.0;
    _seg[0].y[0] = 0.2;
    _seg[0].x[1] = 1.3;
    _seg[0].y[1] = 0.8;
    _seg[0].n_points = 2;
    _seg[0].r = 0.2;
    _seg[0].vvd_index = 0;

    _seg[1].x[0] = 1.3;
    _seg[1].y[0] = 0.2;
    _seg[1].x[1] = 2.7;
    _seg[1].y[1] = 0.8;
    _seg[1].n_points = 2;
    _seg[1].l = 0.1;
    _seg[1].r = 0.2;
    _seg[1].vvd_index = 1;

    _seg[2].x[0] = 2.7;
    _seg[2].y[0] = 0.2;
    _seg[2].x[1] = 3.9;
    _seg[2].y[1] = 0.8;
    _seg[2].n_points = 2;
    _seg[2].l = 0.2;
    _seg[2].vvd_index = 2;

    _olaBuffer = new obOlaBuffer(8*1024);

    update_concat_zone (_seg);
}


int MBROLOIDSynth::runSynth ()
{
    printf ("currentPos %f\n", _currentPos);
    if (_currentIndex > _seg_count - 1)
    {
        printf ("END OF FILE\n");
        return 0;
    }
    float f0 = 200;
    double y_pos =
            interp_linear (_seg[_currentIndex].x, _seg[_currentIndex].y,
                           _seg[_currentIndex].n_points, _currentPos);
    int idx = _seg[_currentIndex].vvd_index;
    _vvd->selectVVD (idx);

    float framePeriod = _vvd->getFramePeriod ();
    /////////
    float fractionalIndex = y_pos * 1000.0 / framePeriod;

    if (_vvd->getSegment (fractionalIndex, _vvdData) == false)
    {
        printf ("PANIC: cannot read from VVD\n");
        exit (1);
    }

    uncompressVVDFrame (_spectrogram, _aperiodicity);

    if (_currentPos >= _left && _concat == false && _lastConcat == false)
    {
        printf ("START CONCAT %f %f %f\n", _left, _mid, _right);

        double y_left =
                interp_linear (_seg[_currentIndex].x, _seg[_currentIndex].y,
                               _seg[_currentIndex].n_points, _mid);
        double y_right =
                interp_linear (_seg[_currentIndex + 1].x, _seg[_currentIndex + 1].y,
                _seg[_currentIndex + 1].n_points, _mid);
        float fractionalIndex_left = y_left * 1000.0 / framePeriod;
        float fractionalIndex_right = y_right * 1000.0 / framePeriod;

        _vvd->selectVVD (_seg[_currentIndex].vvd_index);
        if (_vvd->getSegment (fractionalIndex_left, _vvdData) == false)
        {
            printf ("PANIC: cannot read from VVD [L]\n");
            exit (1);
        }

        _vvd->selectVVD (_seg[_currentIndex + 1].vvd_index);
        if (_vvd->getSegment (fractionalIndex_right, _vvdData) == false)
        {
            printf ("PANIC: cannot read from VVD [R]\n");
            exit (1);
        }
        uncompressVVDFrame (_rightSpectrogram, _rightAperiodicity);

        _concat = true;
    }

    if (_concat)
    {
        if (_currentPos < _mid)
        {
            double factor;
            factor = segment_interp (_currentPos, _left, _mid, 1.0);
            printf ("%f\n", factor);
            for (int i = 0; i < _fftSize / 2 + 1; i++)
            {
                _spectrogram[i] =
                        _spectrogram[i] * (1 - factor) +
                        _rightSpectrogram[i] * factor;
                _aperiodicity[i] =
                        _aperiodicity[i] * (1 - factor) +
                        _rightAperiodicity[i] * factor;
            }
        }
    }

    if (_concat == true && _currentPos >= _right)
    {
        _concat = false;
        printf ("END CONCAT %i\n", _currentIndex);
        if (_currentIndex < _seg_count - 1)
            update_concat_zone (&_seg[_currentIndex]);
        else
            _lastConcat = true;
    }


    int noise_size = ((float) _fs) / f0;
    double current_vuv = 1.0;	//only vowels

    SynthesisOneImpulseResponse (_fftSize,
                                 noise_size,
                                 current_vuv,
                                 _spectrogram,
                                 _aperiodicity,
                                 &_forward_real_fft,
                                 &_inverse_real_fft,
                                 &_minimum_phase,
                                 _periodicResponse,
                                 _aperiodicResponse,
                                 _response);
                                 
    for(int i=0;i<_fftSize;i++)
    {
		_response[i]*=0.8;
	}									

    float period=1.0/f0;
    int periodSamples = _fs*period;
    _olaBuffer->ola(_response, _fftSize, periodSamples );
    _currentPos += period;

    double currentEnd = X_END (_seg[_currentIndex]);
    if (_currentPos > currentEnd)
    {
        _currentIndex++;
    }

    printf ("END step\n");
    return 1;

}

void MBROLOIDSynth::drain(SNDFILE *sf)
{
	float buffer[2*1024];
	if(_olaBuffer->isFilled(4*1024))
	{
		_olaBuffer->pop(buffer,2*1024);
		sf_write_float (sf, buffer, 2*1024);
	}
    
}

/////////-----------------------------------

int main ()
{
    MBROLOIDSynth synth;
    synth.init();
    synth.prepareSynth();
    
    SF_INFO info = { 0 };
    info.samplerate = synth.sampleRate();
    info.channels = 1;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE *sf = sf_open ("test_synth.wav", SFM_WRITE, &info);
    
    while(synth.runSynth()) synth.drain(sf);
    synth.drain(sf);
    
    sf_close (sf);
}
