/* Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty. */

#define MIDI_STATUS(b) (((b) >> 4) & 0x0F)
#define MIDI_CHANNEL(b) ((b) & 0x0F)
#define MIDI_NOTEOFF 0x8
#define MIDI_NOTEON 0x9
