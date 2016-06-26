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

#include "sekai/VVDReader.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <world/common.h>


#define printf(...)  XXXXXXX

//FIXME
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
//

//DO NOT CHANGE
#define VVD_MAGIC 1430997325


struct common_data
{
  int fs;//->FFT_SIZE
  float frame_period;
  int cepstrum_length;
};

#define COMMON_SIZE (sizeof(struct common_data))

////FIX DONE

VVDReader::VVDReader() : vvdconfig(NULL), selectedFile(NULL), selectedIndex(-1), data_ceil(NULL), data_floor(NULL)
{
  //ctor
}

VVDReader::~VVDReader()
{
  //dtor
}

bool VVDReader::addVVD(const std::string& fileName)
{
  FILE* f = fopen(fileName.c_str(),"r");
  if(f)
    {
      struct vvd_header header;
      int result = fread (&header,1,sizeof(header),f);
      if (result==sizeof(header) && header.magic==VVD_MAGIC)
        {
	  if(files.size()==0)
            {
	      vvdconfig = (struct common_data*) new char[COMMON_SIZE];
	      memcpy(vvdconfig,&header.fs,COMMON_SIZE);
	      int frameSize = 1+2*vvdconfig->cepstrum_length;
	      data_floor = new float[frameSize];
	      data_ceil  = new float[frameSize];
            }
	  else
            {
	      if(memcmp(vvdconfig,&header.fs,COMMON_SIZE)!=0)
                {
		  fclose(f);
		  return false;
                }
            }
	  fprintf(stderr,"add vvd file %s len: %i\n",fileName.c_str(),header.f0_length);
	  fileInfo info;
	  info.length=header.f0_length;
	  info.fileName=fileName;
	  files.push_back(info);
	  if(selectedFile) fclose(selectedFile);
	  selectedFile = f;
	  selectedIndex = files.size()-1;
	  return true;

        }
      else
        {
	  fclose(f); //invalid header
	  return false;
        }
    }
  return false;//file does not exist
}

bool VVDReader::selectVVD(int index)
{
  if(selectedIndex == index) return true;
  if(selectedFile) fclose(selectedFile);
  if(index>files.size()) return false;
  selectedIndex = index;
  selectedFile = fopen(files[index].fileName.c_str(),"r");
  if(!selectedFile) return false;
  return true;
}

int VVDReader::getFrameSize()
{
  if(files.size()==0) return 0;
  return sizeof(float)+2*vvdconfig->cepstrum_length*sizeof(float);
}

bool VVDReader::getSegment(int index,void* data)
{
  if(selectedFile==NULL) return false;
  int frame_size = getFrameSize();
  if(index>=files[selectedIndex].length) return false;
  if(fseek(selectedFile,sizeof(struct vvd_header)+index*frame_size,SEEK_SET)!=0) return false;
  int result = fread (data,1,frame_size,selectedFile);
  if(result==frame_size) return true;
  return false;
}

bool VVDReader::getSegment(float fractionalIndex,void* data)
{
  int f0_length = files[selectedIndex].length;
  int frame_floor = MyMinInt(f0_length - 1, static_cast<int>(floor(fractionalIndex)));
  int frame_ceil = MyMinInt(f0_length - 1, static_cast<int>(ceil(fractionalIndex)));
  double interpolation = fractionalIndex - frame_floor;
    
    

  if (frame_floor == frame_ceil) {
    bool b = getSegment(frame_floor,data);
    float* chunk=(float*) data;
    //if(chunk[0]>100) fprintf(stderr,"error reading frame %f\n",chunk[0]);
    return b;
  } else {
    bool b1=getSegment(frame_floor,data_floor);
    if(b1==false) return false;
    bool b2=getSegment(frame_ceil,data_ceil);
    if(b2==false) return false;
    float* data_ret = (float*)data;
    for(int i=0;i<2*vvdconfig->cepstrum_length+1;i++)
      {
	data_ret[i] = (1.0 - interpolation) * data_floor[i] + interpolation * data_ceil[i];
      }
    return true;
  }
}

float VVDReader::getSelectedLength()
{
  if(files.size()==0) return 0;
  if(selectedFile==NULL) return 0;
  return files[selectedIndex].length * vvdconfig->frame_period * 0.001;
}

int VVDReader::getSamplerate()
{
  return vvdconfig->fs;
}

float VVDReader::getFramePeriod()
{
  return vvdconfig->frame_period;
}

int VVDReader::getCepstrumLength()
{
  return vvdconfig->cepstrum_length;
}


