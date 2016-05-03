#include "world/WorldContext.h"
class SekaiContext : public WorldContext
{
	public:
		int cepstrum_length;
		float** mel_cepstrum1;
		float** mel_cepstrum2;
		void Decompress();
		void Compress();

};
