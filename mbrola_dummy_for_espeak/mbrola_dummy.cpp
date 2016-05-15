/*
  Copyleft 2013-2016 Tobias Platen

  This program is a free software replacement for MBROLA,
  you can redistribute it and/or modify it (and its voices)
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <string>
#include <map>
#define printf printf_is_not_allowed

inline int MyMax(int x, int y) {
    return x > y ? x : y;
}
inline double MyMax(double x, double y) {
    return x > y ? x : y;
}
inline int MyMin(int x, int y) {
    return x < y ? x : y;
}
inline double MyMin(double x, double y) {
    return x < y ? x : y;
}



#pragma mark wavwrite for streaming output

void wavwrite_header(FILE* fp, int x_length, int fs, int nbit) {

    char text[4] = {'R', 'I', 'F', 'F'};
    uint32_t long_number = 36 + x_length * 2; //ignored by espeak
    fwrite(text, 1, 4, fp);
    fwrite(&long_number, 4, 1, fp);

    text[0] = 'W';
    text[1] = 'A';
    text[2] = 'V';
    text[3] = 'E';
    fwrite(text, 1, 4, fp);
    text[0] = 'f';
    text[1] = 'm';
    text[2] = 't';
    text[3] = ' ';
    fwrite(text, 1, 4, fp);

    //header part 1

    long_number = 16;
    fwrite(&long_number, 4, 1, fp);
    int16_t short_number = 1;
    fwrite(&short_number, 2, 1, fp);
    short_number = 1;
    fwrite(&short_number, 2, 1, fp);

    long_number = fs;
    fwrite(&long_number, 4, 1, fp);

    long_number = fs * 2;
    fwrite(&long_number, 4, 1, fp);
    short_number = 2;
    fwrite(&short_number, 2, 1, fp);
    short_number = 16;
    fwrite(&short_number, 2, 1, fp);

    text[0] = 'd';
    text[1] = 'a';
    text[2] = 't';
    text[3] = 'a';
    fwrite(text, 1, 4, fp);


    long_number = x_length * 2;
    fwrite(&long_number, 4, 1, fp);
}

void wavwrite_data(FILE* fp,int x_length,double* x)
{

    int16_t tmp_signal;
    for (int i = 0; i < x_length; ++i) {
        tmp_signal = static_cast<int16_t>(MyMax(-32768,
                                                MyMin(32767, static_cast<int>(x[i] * 32767))));
        fwrite(&tmp_signal, 2, 1, fp);
    }
    fflush(stdout);
}

void wavwrite_data2(FILE* fp,int x_length,float* x)
{

    int16_t tmp_signal;
    for (int i = 0; i < x_length; ++i) {
        tmp_signal = static_cast<int16_t>(MyMax(-32768,
                                                MyMin(32767, static_cast<int>(x[i] * 32767))));
        fwrite(&tmp_signal, 2, 1, fp);
    }
    fflush(stdout);
}

#pragma mark reading pho files and pho datastructs

#define MAXVALUES 100

int read_pho_line(char* line,int* values)
{
    int i;
    for(i=0; i<MAXVALUES; i++)
    {
        values[i] = 0;
    }
    int n=strlen(line);
    int nValues=0;
    for(i=0; i<n; i++) if(line[i]==' ' || line[i]=='\t') line[i]=0;

    for(i=0; i<n-1; i++)
    {
        if(line[i]==0)
        {
            while (line[i+1]==0 && i<n-1) i++;

            values[nValues] = atoi(&line[i+1]);
            nValues++;
        }
    }
    return nValues;
}



void drain_buffers(FILE* outfile)
{
	//TODO
}

void mbro_core(char* voicebank_path,char* inphofile,char* outwavfile)
{
    FILE* phofile;
    FILE* outfile;

   // int tmp_buffer_len = 256;
   // double* tmp_buffer = new double[tmp_buffer_len];

    int ready = 0;
    int input_pos = 0;
    int output_pos = 0;

    if(strcmp(inphofile,"-")==0) phofile = stdin;
    else phofile = fopen(inphofile,"r");

    if(strcmp(outwavfile,"-.wav")==0) outfile = stdout;
    else outfile = fopen(outwavfile,"w");

    char line [1000];
    char last_symbol[8];
    int line_counter=0;
    int pos_ms = 0;
    int last_pos_ms = 0;
    int start_time = 0;

    int left_part = -1;
    int right_part = -1;

    while(fgets(line,sizeof(line),phofile)!= NULL) { /* read a line from a file */

        line[strlen(line)-1]=0; // trim newline

        int vb_samplerate = 16000;//FIXME: hardcoded

        if(strcmp("#",line)==0 && ready==0)
        {
            wavwrite_header(outfile,-1,vb_samplerate,16);
            fprintf(stderr,"#start\n");
            fflush(outfile);
            ready = 1;
            continue;
        }
        if(ready==0)
        {
            wavwrite_header(outfile,-1,vb_samplerate,16);
            fprintf(stderr,"#start2\n");
            fflush(outfile);
            ready = 1;
        }
        if(strlen(line) && ready)
        {
            int values[MAXVALUES];

            int count = read_pho_line(line,values);
            if(count==0) {fprintf(stderr,"debug: %s\n",line); continue;}

            int pitch_count = (count-1)/2;
            assert(pitch_count>=0);


            ////            int noise_length = values[0]*0.001*vb_samplerate;
            //wavwrite_noise(outfile,noise_length);

            if(line_counter>0) {
                fprintf(stderr,"DIPH: %s-%s\n",last_symbol,line);                
            }
            else //special case: prepare first segment
            {
                left_part = values[0];
            }
            strcpy(last_symbol,line);

            if(pitch_count==0) {
                fprintf(stderr,"time: %i pitch undefined\n",pos_ms);
            }
            else for(int i=0;i<pitch_count;i++)
            {
                int timestamp = pos_ms+values[0]*0.01*values[i*2+1];
                fprintf(stderr,"time: %i pitch %i\n",timestamp,values[i*2+2]);
            }

            last_pos_ms = pos_ms;
            pos_ms += values[0];

            line_counter++;

            drain_buffers(outfile);

            //TODO -- drain buffers if possible -- run synth and output

        }

    }
    {
        fprintf(stderr,"cleanup -- line counter %i pos_ms: %i\n",line_counter,pos_ms);
        //flush all buffers -- do not feed any new commands
        drain_buffers(outfile);
    }
}

int main(int argc,char** argv)
{

    int nopts = 0;
    for(int i=0; i<argc; i++)
    {
        if(strcmp("-e",argv[i])==0) nopts++;  //-e    = IGNORE fatal errors on unkown diphone
        if(strcmp("-v",argv[i])==0) nopts+=2; //-v VR = VOLUME ratio, float ratio applied to ouput samples, currently ignored
    }
    if (argc-nopts-1 == 3)
    {
        mbro_core(argv[1+nopts],argv[2+nopts],argv[3+nopts]);
    }
    else
    {
        fprintf(stderr,"this program is a dummy for use with eSpeak, currently only allowing transcription\n");
    }
    return 0;
}

//units -- move to other header

typedef int samples_t;
typedef int frames_t;
typedef float milliseconds_t;
