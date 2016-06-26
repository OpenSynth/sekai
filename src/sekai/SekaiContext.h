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

#ifndef SEKAI_CONTEXT_INCLUDED
#define SEKAI_CONTEXT_INCLUDED
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
#endif
