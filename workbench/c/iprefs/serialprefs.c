/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 0
#include <aros/debug.h>

#include <prefs/prefhdr.h>
#include <prefs/serial.h>

static const ULONG buffersizes[] = 
{
	512,
	1024,
	2048,
	4096,
	8000,
	16000,
	-1
};

static LONG stopchunks[] =
{
    ID_PREF, ID_SERL
};

/*********************************************************************************************/

void SerialPrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;	
    struct SerialPrefs *serialprefs;

    D(bug("In IPrefs:SerialPrefs_Handler\n"));
    D(bug("filename=%s\n",filename));
    
    iff = CreateIFF(filename, stopchunks, 1);
    
    if (iff) {
        serialprefs = LoadChunk(iff, sizeof(struct SerialPrefs));
	if (serialprefs) {
	    struct Preferences prefs;
	    ULONG index = 0;
		
	    GetPrefs(&prefs, sizeof(prefs));

	    while (-1 != buffersizes[index]) {
		if (buffersizes[index] == serialprefs->sp_InputBuffer)
		    break;
		index++;
	    }

	    if (-1 == buffersizes[index])
		index = 0;

	    D(bug("Setting new serial prefs.\n"));
	    D(bug("Setting baudrate to %d\n",serialprefs->sp_BaudRate));
	    D(bug("Setting receive buffer size to %d\n",buffersizes[index]));
	    D(bug("Setting read bit len to %d\n",8-serialprefs->sp_BitsPerChar));
	    D(bug("Setting write bit len to %d\n",8-serialprefs->sp_BitsPerChar));
	    D(bug("Setting stop bits to %d\n",1+serialprefs->sp_StopBits));

	    prefs.BaudRate   =  serialprefs->sp_BaudRate;
	    prefs.SerRWBits  = (serialprefs->sp_BitsPerChar << 4) | serialprefs->sp_BitsPerChar;
	    prefs.SerStopBuf = (serialprefs->sp_StopBits    << 4) | index;
	    prefs.SerParShk  = (serialprefs->sp_Parity      << 4) | serialprefs->sp_InputHandshake;

	    SetPrefs(&prefs, sizeof(prefs), TRUE);
	    FreeVec(serialprefs);
	}
	KillIFF(iff);
    }
}
