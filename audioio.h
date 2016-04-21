//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//-----------------------------------------------------------------------------
#ifndef WORLD_AUDIOIO_H_
#define WORLD_AUDIOIO_H_

#ifdef __cplusplus
extern "C" {
#endif

double* wavReadMono(char* fileName,int* samplerate,int* length);
bool wavWriteMono(char* fileName,int samplerate,int length,double* samples);

#ifdef __cplusplus
}
#endif

#endif  // WORLD_AUDIOIO_H_
