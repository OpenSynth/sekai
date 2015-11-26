#include <math.h>
const static double A4Frequency = 440.0;
const static double A4Note = 69.0;

//based on https://github.com/haruneko/SimpleSynthesizer/blob/master/core/org/stand/util/MusicalNote.{h,cpp}

double frequencyFromNote(float note)
{
  if(note==0) return 0;
  return pow(2.0, (note - A4Note) / 12.0) * A4Frequency;
}

float noteFromFrequency(double frequency)
{
  if(frequency==0) return 0;
  return A4Note + 12.0 * log2(frequency / A4Frequency);
}


