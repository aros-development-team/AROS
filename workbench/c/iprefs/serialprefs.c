/*
    (C) 2001 AROS - The Amiga Research OS
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

/*********************************************************************************************/

void SerialPrefs_Handler(STRPTR filename)
{	
	static struct SerialPrefs serialprefs;
	struct MsgPort * SerPort;
	D(bug("In IPrefs:SerialPrefs_Handler\n"));
	D(bug("filename=%s\n",filename));
	SerPort = CreatePort("IPrefsSerial",0);
	if (NULL != SerPort) {
		struct IOExtSer * ioes = (struct IOExtSer *)CreateExtIO(SerPort, sizeof(struct IOExtSer));
		if (NULL  != ioes) {
			if (0 == OpenDevice("serial.device",0,(struct IORequest *)ioes,0)) {
				if (TRUE == LoadSerialPrefs(filename, &serialprefs)) {
					D(bug("Setting new serial prefs."));
					ioes->IOSer.io_Command = SDCMD_SETPARAMS;
					D(bug("Setting baudrate to %d\n",serialprefs.sp_BaudRate));
					ioes->io_Baud = serialprefs.sp_BaudRate;
					D(bug("Setting receive buffer size to %d\n",serialprefs.sp_InputBuffer));
					ioes->io_RBufLen  = serialprefs.sp_InputBuffer;
					D(bug("Setting read len to %d\n",serialprefs.sp_BitsPerChar));
					ioes->io_ReadLen   = serialprefs.sp_BitsPerChar;
					D(bug("Setting write len to %d\n",serialprefs.sp_BitsPerChar));
					ioes->io_WriteLen  = serialprefs.sp_BitsPerChar;
					D(bug("Setting stop bits to %d\n",serialprefs.sp_StopBits));
					ioes->io_StopBits  = serialprefs.sp_StopBits;
					DoIO((struct IORequest *)ioes);
					if (0 != ((struct IORequest *)ioes)->io_Error) {
						D(bug("Could not change settings on serial device!\n"));
					} else {
						D(bug("Successfully changed settings on serial device!\n"));
					}
				}
				CloseDevice((struct IORequest *)ioes);
			}
			DeleteExtIO((struct IORequest *)ioes);
		}
		DeletePort(SerPort);
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
