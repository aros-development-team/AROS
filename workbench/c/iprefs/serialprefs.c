/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 1
#include <aros/debug.h>

#include <prefs/prefhdr.h>
#include <prefs/serial.h>

static BOOL LoadSerialPrefs(STRPTR filename, struct SerialPrefs * serialprefs);

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

/*********************************************************************************************/

void SerialPrefs_Handler(STRPTR filename)
{	
	static struct SerialPrefs serialprefs;
	D(bug("In IPrefs:SerialPrefs_Handler\n"));
	D(bug("filename=%s\n",filename));
	if (TRUE == LoadSerialPrefs(filename, &serialprefs)) {
		struct Preferences prefs;
		ULONG index = 0;
		GetPrefs(&prefs, sizeof(prefs));

		while (-1 != buffersizes[index]) {
			if (buffersizes[index] == serialprefs.sp_InputBuffer)
				break;
			index++;
		}

		if (-1 == buffersizes[index])
			index = 0;

		D(bug("Setting new serial prefs.\n"));
		D(bug("Setting baudrate to %d\n",serialprefs.sp_BaudRate));
		D(bug("Setting receive buffer size to %d\n",buffersizes[index]));
		D(bug("Setting read bit len to %d\n",8-serialprefs.sp_BitsPerChar));
		D(bug("Setting write bit len to %d\n",8-serialprefs.sp_BitsPerChar));
		D(bug("Setting stop bits to %d\n",1+serialprefs.sp_StopBits));

		prefs.BaudRate   =  serialprefs.sp_BaudRate;
		prefs.SerRWBits  = (serialprefs.sp_BitsPerChar << 4) | serialprefs.sp_BitsPerChar;
		prefs.SerStopBuf = (serialprefs.sp_StopBits    << 4) | index;
		prefs.SerParShk  = (serialprefs.sp_Parity      << 4) | serialprefs.sp_InputHandshake;

		SetPrefs(&prefs, sizeof(prefs), TRUE);
	}
}
/*********************************************************************************************/

static BOOL LoadSerialPrefs(STRPTR filename, struct SerialPrefs * serialprefs)
{
    struct IFFHandle 	    	*iff;    
    BOOL    	    	    	retval = FALSE;
    
    D(bug("LoadPrefs: Trying to open \"%s\"\n", filename));
    
    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE)))
	{
    	    D(bug("LoadPrefs: stream opened.\n"));
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_READ))
	    {
    	    	D(bug("LoadPrefs: OpenIFF okay.\n"));
		
	    	if (!StopChunk(iff, ID_PREF, ID_SERL))
		{
    	    	    D(bug("LoadPrefs: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("LoadPrefs: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(struct SerialPrefs))
			{
   	    	    	    D(bug("LoadPrefs: ID_SERL chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, serialprefs, sizeof(struct SerialPrefs)) == sizeof(struct SerialPrefs))
			    {
   	    	    	    	D(bug("LoadSerialPrefs: Reading chunk successful.\n"));

   	    	    	    	D(bug("LoadSerialPrefs: Everything okay :-)\n"));
				
				retval = TRUE;
			    }
			}
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!StopChunk(iff, ID_PREF, ID_SERL)) */
		
	    	CloseIFF(iff);
				
	    } /* if (!OpenIFF(iff, IFFF_READ)) */
	    
	    Close((BPTR)iff->iff_Stream);
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE))) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    return retval;
}
