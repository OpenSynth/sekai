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
//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//
// .wav input/output functions were modified for compatibility with C language.
// Since these functions (wavread() and wavwrite()) are roughly implemented,
// we recommend more suitable functions provided by other organizations.
// This file is independent of WORLD project and for the test.cpp.
//-----------------------------------------------------------------------------
#include "audioio.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sndfile.h>
 
double* wavReadMono(char* fileName,int* samplerate,int* length)
{
  SF_INFO info;
  memset(&info,0,sizeof(info));
  SNDFILE* sf = sf_open(fileName,SFM_READ,&info);
  *samplerate = info.samplerate;
  *length = info.frames;
  if(info.channels!=1) return NULL;
  double* ret = new double[*length];
  sf_read_double(sf,ret,*length);
  sf_close(sf);
  return ret;
}

bool wavWriteMono(char* fileName,int samplerate,int length,double* samples)
{
  SF_INFO info;
  memset(&info,0,sizeof(info));
  info.format =  SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  info.samplerate = samplerate;
  info.channels = 1;
  SNDFILE* sf = sf_open(fileName,SFM_WRITE,&info);
  int count = sf_write_double(sf,samples,length);
  sf_close(sf);
  return count==length;
}
