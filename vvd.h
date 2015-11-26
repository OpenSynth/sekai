struct vvd_header
{
	int magic;
	int version;
	int f0_length;
	int fs;//->FFT_SIZE
	float frame_period;
	int cepstrum_length;
	int flags;
};
