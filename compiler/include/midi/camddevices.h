#ifndef MIDI_CAMDDEVICES_H
#define MIDI_CAMDDEVICES_H

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/


#ifndef EXEC_TYPES_H
#  include <exec/types.h>
#endif

#ifndef LIBCORE_COMPILER_H
#  include <libcore/compiler.h>
#endif


struct MidiPortData{
	void (*ActivateXmit)(void);
};

struct MidiDeviceData{
	ULONG Magic;

	char *Name;
	char *IDString;

	UWORD Version;
	UWORD Revision;

	BOOL (*Init)(void);
	void (*Expunge)(void);
	struct MidiPortData *(ASM *OpenPort)(
	 	REG(a3) struct MidiDeviceData *data,
		REG(d0) LONG portnum,
		REG(a0) APTR transmitfunc,
		REG(a1) APTR recievefunc,
		REG(a2) APTR userdata
	);
	void (ASM *ClosePort)(
	  	REG(a3) struct MidiDeviceData *data,
		REG(d0) LONG portnum
	);

	UBYTE NPorts;
	UBYTE Flags;
};


#define MDD_Magic ((ULONG)'M'<<24|(ULONG)'D'<<16|'E'<<8|'V')


#endif


