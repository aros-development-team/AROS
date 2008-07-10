/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <aros/macros.h>

/* #define DEBUG 1 */
#include <aros/debug.h>

#include <proto/iffparse.h>
#include <proto/dos.h>

#include <prefs/prefhdr.h>

#include "global.h"


/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

static struct SerialPrefs   restore_prefs;
struct SerialPrefs          serialprefs;
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

VOID CleanupPrefs(void)
{
    D(bug("[serial prefs] CleanupPrefs\n"));
    if (mempool) DeletePool(mempool);
}

/*********************************************************************************************/

VOID CopyPrefs(struct SerialPrefs *s, struct SerialPrefs *d)
{
    CopyMem(s, d, sizeof(struct SerialPrefs));
}

VOID BackupPrefs(void)
{
    CopyPrefs(&serialprefs, &restore_prefs);
}

VOID RestorePrefs(void)
{
    CopyPrefs(&restore_prefs, &serialprefs);
}

/*********************************************************************************************/

/* 1 is success */

ULONG InitPrefs(STRPTR filename, BOOL use, BOOL save)
{
    D(bug("[serial prefs] InitPrefs\n"));
    mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, 2048, 2048);
    if (!mempool) 
    {
	ShowMsg("Out of memory!");
	return 0;
    }

    if (!LoadPrefs(filename))
    {
    	if (!DefaultPrefs())
	{
	    CleanupPrefs();
	    ShowMsg("Panic! Cannot setup default prefs!");
	    return 0;
	}
    }
    
    restore_prefs = serialprefs;
    
    if (use || save)
    {
    	SavePrefs((CONST STRPTR) CONFIGNAME_ENV);
    }
    
    if (save)
    {
    	SavePrefs((CONST STRPTR) CONFIGNAME_ENVARC);
    }
    
    if (use || save) CleanupPrefs();

    return 1;
}

/*********************************************************************************************/

BOOL LoadPrefsFH(BPTR fh)
{
    static struct SerialPrefs loadprefs;
    struct IFFHandle 	    	*iff;    
    BOOL    	    	    	retval = FALSE;
    
    
    if ((iff = AllocIFF()))
    {
    	iff->iff_Stream = (IPTR)fh;
	    
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

			    serialprefs = loadprefs;
			    
			    D(bug("LoadPrefs: Everything okay :-)\n"));
			    
			    retval = TRUE;
			}
		    }
		    
		} /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		
	    } /* if (!StopChunk(iff, ID_PREF, ID_SERL)) */
	    
	    CloseIFF(iff);
			    
	} /* if (!OpenIFF(iff, IFFF_READ)) */
	
    
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    return retval;
}

BOOL LoadPrefs(STRPTR filename) 
{
    BPTR fh;
    BOOL ret;

    D(bug("[serial prefs] LoadPrefsFH: Trying to open \"%s\"\n", filename));

    fh=Open(filename, MODE_OLDFILE);

    if(!fh) return FALSE;

    ret=LoadPrefsFH(fh);

    Close(fh);
    return ret;
}
/*********************************************************************************************/

BOOL SavePrefsFH(BPTR fh)
{
    static struct SerialPrefs 	saveprefs;
    struct IFFHandle 	     	*iff;    
    BOOL    	    	    	retval = FALSE, delete_if_error = FALSE;
    
    saveprefs = serialprefs;
    
    
    D(bug("SavePrefsFH: fh: %lx\n", fh));
    //if ((iff->iff_Stream = (IPTR)Open(filename, MODE_NEWFILE)))
    
    if ((iff = AllocIFF()))
    {
	iff->iff_Stream = (IPTR) fh;
	D(bug("SavePrefsFH: stream opened.\n"));
	    
	    delete_if_error = TRUE;
	    
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_WRITE))
	    {
    	    	D(bug("SavePrefsFH: OpenIFF okay.\n"));
		
		if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
		{
    	    	    D(bug("SavePrefsFH: PushChunk(FORM) okay.\n"));
		    
		    if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
		    {
		    	struct FilePrefHeader head;

    	    	    	D(bug("SavePrefsFH: PushChunk(PRHD) okay.\n"));
			
			head.ph_Version  = PHV_CURRENT; 
			head.ph_Type     = 0;
			head.ph_Flags[0] =
			head.ph_Flags[1] =
			head.ph_Flags[2] =
			head.ph_Flags[3] = 0;
			
			if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
			{
    	    	    	    D(bug("SavePrefsFH: WriteChunkBytes(PRHD) okay.\n"));
			    
			    PopChunk(iff);
			    
			    if (!PushChunk(iff, ID_PREF, ID_SERL, sizeof(struct SerialPrefs)))
			    {
    	    	    	    	D(bug("SavePrefsFH: PushChunk(LCLE) okay.\n"));
				
			    	if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
				{
   	    	    	    	    D(bug("SavePrefsFH: WriteChunkBytes(SERL) okay.\n"));
  	    	    	    	    D(bug("SavePrefsFH: Everything okay :-)\n"));
				    
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
	    
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    #if 0
    if (!retval && delete_if_error)
    {
    	DeleteFile(filename);
    }
    #endif
	    
    
    return retval;    
}

BOOL SavePrefs(CONST STRPTR filename) 
{
    BPTR fh;
    BOOL ret;

    D(bug("[serial prefs] SavePrefs: Trying to open \"%s\"\n", filename));

    fh=Open(filename, MODE_NEWFILE);

    if(fh == NULL) 
    {
      	D(bug("[serial prefs] open \"%s\" failed!\n", filename));
       	return FALSE;
    }

    ret=SavePrefsFH(fh);

    Close(fh);
    return ret;
}

/*********************************************************************************************/

BOOL SaveEnv() {
    BPTR fh;
    BOOL result;

    D(bug("[serial prefs] SaveEnv: Trying to open \"%s\"\n", CONFIGNAME_ENV));

    fh=Open((CONST_STRPTR) CONFIGNAME_ENV, MODE_NEWFILE);

    if(fh == NULL) 
    {
	D(bug("[serial prefs] open \"%s\" failed!\n", CONFIGNAME_ENV));
	return FALSE;
    }

    result=SavePrefsFH(fh);

    Close(fh);

    return result;
}

/*********************************************************************************************/

BOOL DefaultPrefs(void)
{
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

    return TRUE;
}

