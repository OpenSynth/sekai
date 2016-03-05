#define MAXPOINTS 8

typedef struct {
    float x[MAXPOINTS];
    float y[MAXPOINTS];
    float l;
    float r;
    int n_points;
    int vvd_index;
} segment_t;

#define X_END(p) p.x[p.n_points-1]

class MBROLOIDSynth
{
	float _mid;
	float _left;
	float _right;
	bool _concat;
	bool _lastConcat;
	int _currentIndex;
	double _currentPos;
	////////////////////////////////////////////////////

	MinimumPhaseAnalysis _minimum_phase;
	ForwardRealFFT _forward_real_fft;
	InverseRealFFT _inverse_real_fft;
	float* _vvdData;
	int _fs;
	int _fftSize;
	double* _spectrogram;
	double* _aperiodicity;  
	double* _rightSpectrogram;
	double* _rightAperiodicity;    
	double* _periodicResponse;
	double* _aperiodicResponse;
	double* _response;
	double* _olaFrame;
	int _cepstrumLength;
	float _vvdPitch;
	VVDReader* _vvd;

	//input seg
	int _seg_count;
	segment_t _seg[100];
	
	//output buffer
	obOlaBuffer* _olaBuffer;
	
	void uncompressVVDFrame(double* spectrogram,double* aperiodicity);
	void update_concat_zone(segment_t* s);
	
public:
	int init();
	int prepareSynth();
	int runSynth();
	void drain(SNDFILE *sf);
	int sampleRate(){return _fs;}
};
