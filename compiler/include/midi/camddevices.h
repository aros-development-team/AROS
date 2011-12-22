#ifndef MIDI_CAMDDEVICES_H
#define MIDI_CAMDDEVICES_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#  include <exec/types.h>
#endif

#ifndef LIBCORE_COMPILER_H
#  include <libcore/compiler.h>
#endif


struct MidiPortData{
	void (* ASM ActivateXmit)(APTR REG(a2) userdata,ULONG REG(d0) portnum);
};

struct MidiDeviceData{
  ULONG Magic;
  
  char *Name;
  char *IDString;
  
  UWORD Version;
  UWORD Revision;

  /* Called right after LoadSeg() */
  BOOL (ASM *Init)(
		   /* Added. -Kjetil M. */
		   REG(a6) APTR SysBase
		   );
  

  /* Called right before UnLoadSeg() */
  void (*Expunge)(void);

  struct MidiPortData *(ASM *OpenPort)(
	 	REG(a3) struct MidiDeviceData *data,
		REG(d0) LONG portnum,
		REG(a0) ULONG (* ASM transmitfunc)(APTR REG(a2) userdata),
		REG(a1) void (* ASM receivefunc)(UWORD REG(d0) input, APTR REG(a2) userdata),
		REG(a2) APTR userdata
		);

  void (ASM *ClosePort)(
			REG(a3) struct MidiDeviceData *data,
			REG(d0) LONG portnum
			);
  
  /* Number of ports. Cam be set in the Init-function if prefered. */
  UBYTE NPorts;

  /* 0=Old format, 1=new. Aros only supports the new format, so set this flag (ie. Flags=1) for now. */
  UBYTE Flags;
};


#define MDD_Magic ((ULONG)'M'<<24|(ULONG)'D'<<16|'E'<<8|'V')


#endif


