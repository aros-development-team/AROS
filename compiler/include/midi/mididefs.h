#ifndef MIDI_MIDIDEFS_H
#define MIDI_MIDIDEFS_H

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/


#define MIDIHiByte(word) ((word)>>7&0x7f)
#define MIDILoByte(word) ((word)&0x7f)
#define MIDIWord(hi,lo) (((hi)&0x7f)<<7|((lo)&0x7f))


#endif

