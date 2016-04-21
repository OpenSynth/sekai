all: vvd2wav wav2vvd
wav2vvd: wav2vvd_new.cpp
	g++ -g wav2vvd_new.cpp -lworld mfcc.cpp midi.cpp audioio.cpp -lsndfile -o wav2vvd
vvd2wav: vvd2wav_new.cpp
	g++ -g vvd2wav_new.cpp -lworld mfcc.cpp midi.cpp audioio.cpp -lsndfile -o vvd2wav
