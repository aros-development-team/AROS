#ifndef MIDI_MIDIDEFS_H
#define MIDI_MIDIDEFS_H

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/



/*
  This defines are not complete. Please fill in whenever programs
  require them. Look here for documentation:
  http://www.harmony-central.com/MIDI/Doc/doc.html
  
  -Kjetil M.
*/


#define MS_Prog 0xc0
#define MS_Ctrl 0xb0

#define MM_ResetCtrl 0x79
#define MM_AllOff 0x7b

#define MIDIHiByte(word) ((word)>>7&0x7f)
#define MIDILoByte(word) ((word)&0x7f)
#define MIDIWord(hi,lo) (((hi)&0x7f)<<7|((lo)&0x7f))


#endif

