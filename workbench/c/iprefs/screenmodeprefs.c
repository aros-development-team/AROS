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
#include <prefs/screenmode.h>

#include <intuition/iprefs.h>

static BOOL LoadScreenModePrefs(STRPTR filename, struct ScreenModePrefs * serialprefs);

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

void ScreenModePrefs_Handler(STRPTR filename)
{	
    static struct ScreenModePrefs smp;
    D(bug("In IPrefs:ScreenModePrefs_Handler\n"));
    D(bug("filename=%s\n",filename));
	
    if (TRUE == LoadScreenModePrefs(filename, &smp))
    {
        struct IScreenModePrefs i;
	
	i.smp_DisplayID = smp.smp_DisplayID;
	i.smp_Width     = smp.smp_Width;
	i.smp_Height    = smp.smp_Height;
	i.smp_Depth     = smp.smp_Depth;
	i.smp_Control   = smp.smp_Control;
	
        SetIPrefs(&i, sizeof(struct IScreenModePrefs), IPREFS_TYPE_SCREENMODE);
    }
}
/*********************************************************************************************/

static BOOL LoadScreenModePrefs(STRPTR filename, struct ScreenModePrefs * serialprefs)
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
		
	    	if (!StopChunk(iff, ID_PREF, ID_SCRM))
		{
    	    	    D(bug("LoadPrefs: StopChunk okay.\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
    	    	    	D(bug("LoadPrefs: ParseIFF okay.\n"));
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(struct ScreenModePrefs))
			{
   	    	    	    D(bug("LoadPrefs: ID_SERL chunk size okay.\n"));
			    
		    	    if (ReadChunkBytes(iff, serialprefs, sizeof(struct ScreenModePrefs)) == sizeof(struct ScreenModePrefs))
			    {
   	    	    	    	D(bug("LoadScreenModePrefs: Reading chunk successful.\n"));

   	    	    	    	D(bug("LoadScreenModePrefs: Everything okay :-)\n"));
				
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
