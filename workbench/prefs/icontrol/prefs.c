/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/macros.h>

#define DEBUG 0
#include <aros/debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*********************************************************************************************/

#define ARRAY_TO_LONG(x) ( ((x)[0] << 24UL) + ((x)[1] << 16UL) + ((x)[2] << 8UL) + ((x)[3]) )
#define ARRAY_TO_WORD(x) ( ((x)[0] << 8UL) + ((x)[1]) )

#define LONG_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >> 24UL); \
    	    	    	   (y)[1] = (UBYTE)(ULONG)((x) >> 16UL); \
			   (y)[2] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[3] = (UBYTE)(ULONG)((x));

#define WORD_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[1] = (UBYTE)(ULONG)((x));

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

struct FileIControlPrefs
{
    UBYTE   ic_Reserved0[4];
    UBYTE   ic_Reserved1[4];
    UBYTE   ic_Reserved2[4];
    UBYTE   ic_Reserved3[4];
    UBYTE   ic_TimeOut[2];
    UBYTE   ic_MetaDrag[2];
    UBYTE   ic_Flags[4];
    UBYTE   ic_WBtoFront;
    UBYTE   ic_FrontToBack;
    UBYTE   ic_ReqTrue;
    UBYTE   ic_ReqFalse;
};

/*********************************************************************************************/

void InitPrefs(STRPTR filename, BOOL use, BOOL save)
{
    if (!LoadPrefs(filename))
    {
    	if (!DefaultPrefs())
	{
	}
    }
    
    restore_prefs = icontrolprefs;
    
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
}

/*********************************************************************************************/

BOOL LoadPrefs(STRPTR filename)
{
    static struct FileIControlPrefs loadprefs;
    struct IFFHandle 	    	    *iff;    
    BOOL    	    	    	    retval = FALSE;
    
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
		
	    	if (!StopChunk(iff, ID_PREF, ID_ICTL))
		{
    	    	    D(bug("LoadPrefs: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("LoadPrefs: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(loadprefs))
			{
   	    	    	    D(bug("LoadPrefs: ID_ICTL chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, &loadprefs, sizeof(loadprefs)) == sizeof(loadprefs))
			    {
   	    	    	    	D(bug("LoadPrefs: Reading chunk successful.\n"));

    	    	    	    	icontrolprefs.ic_Reserved[0] = ARRAY_TO_LONG(loadprefs.ic_Reserved0);
    	    	    	    	icontrolprefs.ic_Reserved[1] = ARRAY_TO_LONG(loadprefs.ic_Reserved1);
    	    	    	    	icontrolprefs.ic_Reserved[2] = ARRAY_TO_LONG(loadprefs.ic_Reserved2);
    	    	    	    	icontrolprefs.ic_Reserved[3] = ARRAY_TO_LONG(loadprefs.ic_Reserved3);
				icontrolprefs.ic_TimeOut = ARRAY_TO_WORD(loadprefs.ic_TimeOut);
				icontrolprefs.ic_MetaDrag = ARRAY_TO_WORD(loadprefs.ic_MetaDrag);
				icontrolprefs.ic_Flags = ARRAY_TO_LONG(loadprefs.ic_Flags);
				icontrolprefs.ic_WBtoFront = loadprefs.ic_WBtoFront;
				icontrolprefs.ic_FrontToBack = loadprefs.ic_FrontToBack;
				icontrolprefs.ic_ReqTrue = loadprefs.ic_ReqTrue;
				icontrolprefs.ic_ReqFalse = loadprefs.ic_ReqFalse;
				
   	    	    	    	D(bug("LoadPrefs: Everything okay :-)\n"));
				
				retval = TRUE;
			    }
			}
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!StopChunk(iff, ID_PREF, ID_INPT)) */
		
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
    static struct FileIControlPrefs saveprefs;
    struct IFFHandle 	     	    *iff;    
    BOOL    	    	    	    retval = FALSE, delete_if_error = FALSE;
    
    LONG_TO_ARRAY(icontrolprefs.ic_Reserved[0], saveprefs.ic_Reserved0);
    LONG_TO_ARRAY(icontrolprefs.ic_Reserved[1], saveprefs.ic_Reserved1);
    LONG_TO_ARRAY(icontrolprefs.ic_Reserved[2], saveprefs.ic_Reserved2);
    LONG_TO_ARRAY(icontrolprefs.ic_Reserved[3], saveprefs.ic_Reserved3);
    WORD_TO_ARRAY(icontrolprefs.ic_TimeOut, saveprefs.ic_TimeOut);
    WORD_TO_ARRAY(icontrolprefs.ic_MetaDrag, saveprefs.ic_MetaDrag);
    LONG_TO_ARRAY(icontrolprefs.ic_Flags, saveprefs.ic_Flags);
    saveprefs.ic_WBtoFront = icontrolprefs.ic_WBtoFront;
    saveprefs.ic_FrontToBack = icontrolprefs.ic_FrontToBack;
    saveprefs.ic_ReqTrue = icontrolprefs.ic_ReqTrue;
    saveprefs.ic_ReqFalse = icontrolprefs.ic_ReqFalse;

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
			
			head.ph_Version  = 0; // FIXME: shouold be PHV_CURRENT, but see <prefs/prefhdr.h> 
			head.ph_Type     = 0;
			head.ph_Flags[0] =
			head.ph_Flags[1] =
			head.ph_Flags[2] =
			head.ph_Flags[3] = 0;
			
			if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
			{
    	    	    	    D(bug("SavePrefs: WriteChunkBytes(PRHD) okay.\n"));
			    
			    PopChunk(iff);
			    
			    if (!PushChunk(iff, ID_PREF, ID_ICTL, sizeof(saveprefs)))
			    {
    	    	    	    	D(bug("SavePrefs: PushChunk(INPT) okay.\n"));
				
			    	if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
				{
   	    	    	    	    D(bug("SavePrefs: WriteChunkBytes(ICTL) okay.\n"));
  	    	    	    	    D(bug("SavePrefs: Everything okay :-)\n"));
				    
				    retval = TRUE;
				}
				
    			    	PopChunk(iff);

			    } /* if (!PushChunk(iff, ID_PREF, ID_INPT, sizeof(saveprefs))) */
			    			    
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
    icontrolprefs.ic_Reserved[0] = 0;
    icontrolprefs.ic_Reserved[1] = 0;
    icontrolprefs.ic_Reserved[2] = 0;
    icontrolprefs.ic_Reserved[3] = 0;
    icontrolprefs.ic_TimeOut = 50;
    icontrolprefs.ic_MetaDrag = IEQUALIFIER_LCOMMAND;    
    icontrolprefs.ic_Flags = ICF_3DMENUS |
    	    	    	     ICF_MODEPROMOTE | 
    	    	    	     ICF_MENUSNAP |
			     ICF_STRGAD_FILTER |
			     ICF_COERCE_LACE |
			     ICF_OFFSCREENLAYERS;
    			     /* FIXME: check whether ICF_DEFPUBSCREEN is set as default */
    icontrolprefs.ic_WBtoFront = 'N';
    icontrolprefs.ic_FrontToBack = 'M';
    icontrolprefs.ic_ReqTrue = 'V';
    icontrolprefs.ic_ReqFalse = 'B';
    			     
    return TRUE;
}


/*********************************************************************************************/

void RestorePrefs(void)
{
    icontrolprefs = restore_prefs;
}

/*********************************************************************************************/
