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


#define MS_NoteOff 0x80
#define MS_NoteOn 0x90

#define MS_Prog 0xc0
#define MS_Ctrl 0xb0

#define MS_ChanBits 0xf

#define MM_ResetCtrl 0x79
#define MM_AllOff 0x7b

#define MIDIHiByte(word) ((word)>>7&0x7f)
#define MIDILoByte(word) ((word)&0x7f)
#define MIDIWord(hi,lo) (((hi)&0x7f)<<7|((lo)&0x7f))

#define voicemsg(m,ms) ( (((m)->mm_Status)&0xf0) == (ms) )

#define modemsg(m) ( voicemsg(m,0xb0) && (m)->mm_Data1>=0x78)

#if 0
   Taken from the fireworks source. It is not compatible with AmigaOS, and is here only for
   easier defining of noteon, etc.
#  define noteoff(m) ( voicemsg(m,MS_NoteOff) || (voicemsg(m,MS_NoteOn) && (!(m)->mm_Data2)) )
#endif

#define noteon(m) (voicemsg(m,MS_NoteOn) && ((m)->mm_Data2))

#endif

