/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/macros.h>

#define DEBUG 1
#include <aros/debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

static struct SerialPrefs   restore_prefs;
static APTR 	    	    mempool;

/*********************************************************************************************/

#define SER_DEFAULT_BAUDRATE       19200
#define SER_DEFAULT_IBUFFER        512
#define SER_DEFAULT_OBUFFER        512

#define SER_DEFAULT_IHANDSHAKE     HSHAKE_NONE
#define SER_DEFAULT_OHANDSHAKE     HSHAKE_NONE

#define SER_DEFAULT_PARITY         PARITY_NONE
#define SER_DEFAULT_BITS_PER_CHAR  8
#define SER_DEFAULT_STOP_BITS      1

/*********************************************************************************************/


void InitPrefs(STRPTR filename, BOOL use, BOOL save)
{
    mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);
    if (!mempool) Cleanup("Out of memory!");

    if (!LoadPrefs(filename))
    {
    	if (!DefaultPrefs())
	{
	    Cleanup("Panic! Cannot setup default prefs!");
	}
    }
    
    restore_prefs = serialprefs;
    
    if (use || save)
    {
    	SavePrefs(CONFIGNAME_ENV);
    }
    
    if (save)
    {
    	SavePrefs(CONFIGNAME_ENVARC);
    }
    
    if (use || save) Cleanup(NULL);
}

/*********************************************************************************************/

void CleanupPrefs(void)
{
    if (mempool) DeletePool(mempool);
}

/*********************************************************************************************/

BOOL LoadPrefs(STRPTR filename)
{
    static struct SerialPrefs loadprefs;
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
			    
		    	    if (ReadChunkBytes(iff, &loadprefs, sizeof(struct SerialPrefs)) == sizeof(struct SerialPrefs))
			    {
   	    	    	    	D(bug("LoadPrefs: Reading chunk successful.\n"));

    	    	    	    	TellGUI(PAGECMD_PREFS_CHANGING);

    	    	    	    	serialprefs = loadprefs;
				
    	    	    	    	TellGUI(PAGECMD_PREFS_CHANGED);
			    
   	    	    	    	D(bug("LoadPrefs: Everything okay :-)\n"));
				
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

/*********************************************************************************************/

BOOL SavePrefs(STRPTR filename)
{
    static struct SerialPrefs 	saveprefs;
    struct IFFHandle 	     	*iff;    
    BOOL    	    	    	retval = FALSE, delete_if_error = FALSE;
    
    saveprefs = serialprefs;
    
    
    D(bug("SavePrefs: Trying to open \"%s\"\n", filename));
    
    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR)Open(filename, MODE_NEWFILE)))
	{
    	    D(bug("SavePrefs: stream opened.\n"));
	    
	    delete_if_error = TRUE;
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_WRITE))
	    {
    	    	D(bug("SavePrefs: OpenIFF okay.\n"));
		
		if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
		{
    	    	    D(bug("SavePrefs: PushChunk(FORM) okay.\n"));
		    
		    if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
		    {
		    	struct FilePrefHeader head;

    	    	    	D(bug("SavePrefs: PushChunk(PRHD) okay.\n"));
			
			head.ph_Version  = 0; // FIXME: should be PHV_CURRENT, but see <prefs/prefhdr.h> 
			head.ph_Type     = 0;
			head.ph_Flags[0] =
			head.ph_Flags[1] =
			head.ph_Flags[2] =
			head.ph_Flags[3] = 0;
			
			if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
			{
    	    	    	    D(bug("SavePrefs: WriteChunkBytes(PRHD) okay.\n"));
			    
			    PopChunk(iff);
			    
			    if (!PushChunk(iff, ID_PREF, ID_SERL, sizeof(struct SerialPrefs)))
			    {
    	    	    	    	D(bug("SavePrefs: PushChunk(LCLE) okay.\n"));
				
			    	if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
				{
   	    	    	    	    D(bug("SavePrefs: WriteChunkBytes(SERL) okay.\n"));
  	    	    	    	    D(bug("SavePrefs: Everything okay :-)\n"));
				    
				    retval = TRUE;
				}
				
    			    	PopChunk(iff);

			    } /* if (!PushChunk(iff, ID_PREF, ID_SERL, sizeof(struct LocalePrefs))) */
			    			    
			} /* if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head)) */
			else
		    	{
			    PopChunk(iff);
			}
			
		    } /* if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct PrefHeader))) */
		    
		    PopChunk(iff);
		    		    
		} /* if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) */
		
	    	CloseIFF(iff);
				
	    } /* if (!OpenIFF(iff, IFFFWRITE)) */
	    
	    Close((BPTR)iff->iff_Stream);
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_NEWFILE))) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    if (!retval && delete_if_error)
    {
    	DeleteFile(filename);
    }
    
    return retval;    
}

/*********************************************************************************************/

BOOL DefaultPrefs(void)
{
    TellGUI(PAGECMD_PREFS_CHANGING);
    
    serialprefs.sp_Reserved[0]     = 0;
    serialprefs.sp_Reserved[1]     = 0;
    serialprefs.sp_Reserved[2]     = 0;
    serialprefs.sp_Unit0Map        = 0;
    serialprefs.sp_BaudRate        = SER_DEFAULT_BAUDRATE;
    serialprefs.sp_InputBuffer     = SER_DEFAULT_IBUFFER;
    serialprefs.sp_OutputBuffer    = SER_DEFAULT_OBUFFER;
    serialprefs.sp_InputHandshake  = SER_DEFAULT_IHANDSHAKE;
    serialprefs.sp_OutputHandshake = SER_DEFAULT_OHANDSHAKE;
    serialprefs.sp_Parity          = SER_DEFAULT_PARITY;
    serialprefs.sp_BitsPerChar     = SER_DEFAULT_BITS_PER_CHAR;
    serialprefs.sp_StopBits        = SER_DEFAULT_STOP_BITS;

    TellGUI(PAGECMD_PREFS_CHANGED);
    
    return TRUE;
}

/*********************************************************************************************/

void RestorePrefs(void)
{
    TellGUI(PAGECMD_PREFS_CHANGING);
    serialprefs = restore_prefs;
    TellGUI(PAGECMD_PREFS_CHANGED);   
}

/*********************************************************************************************/
