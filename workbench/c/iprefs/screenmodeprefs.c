/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 0
#include <aros/debug.h>

#include <prefs/prefhdr.h>
#include <prefs/screenmode.h>

#include <intuition/iprefs.h>

static LONG stopchunks[] =
{
    ID_PREF, ID_SCRM
};

/*********************************************************************************************/

void ScreenModePrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;
    struct ScreenModePrefs *smp;

    D(bug("In IPrefs:ScreenModePrefs_Handler\n"));
    D(bug("filename=%s\n",filename));

    iff = CreateIFF(filename, stopchunks, 1);
    
    if (iff) {
        while(ParseIFF(iff, IFFPARSE_SCAN) == 0) {
            smp = LoadChunk(iff, sizeof(struct ScreenModePrefs), MEMF_ANY);
	    if (smp) {
                struct IScreenModePrefs i;
	
	        i.smp_DisplayID = GET_LONG(smp->smp_DisplayID);
	        i.smp_Width     = GET_WORD(smp->smp_Width);
	        i.smp_Height    = GET_WORD(smp->smp_Height);
	        i.smp_Depth     = GET_WORD(smp->smp_Depth);
	        i.smp_Control   = AROS_BE2WORD(smp->smp_Control);
	        D(bug("[ScreenModePrefs] ModeID: 0x%08lX, Size: %dx%d, Depth: %d, Control: 0x%08lX\n",
	              i.smp_DisplayID, i.smp_Width, i.smp_Height, i.smp_Depth, i.smp_Control));
	
                SetIPrefs(&i, sizeof(struct IScreenModePrefs), IPREFS_TYPE_SCREENMODE);
	        FreeVec(smp);
	    }
	}
	KillIFF(iff);
    }
}
