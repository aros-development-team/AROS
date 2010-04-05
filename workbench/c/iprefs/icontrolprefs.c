/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <prefs/prefhdr.h>
#include <prefs/icontrol.h>
#include <intuition/iprefs.h>

#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

struct FileIControlPrefs
{
    LONG    ic_Reserved[4];
    UBYTE   ic_TimeOut[2];
    UBYTE   ic_MetaDrag[2];
    UBYTE   ic_Flags[4];
    UBYTE   ic_WBtoFront;
    UBYTE   ic_FrontToBack;
    UBYTE   ic_ReqTrue;
    UBYTE   ic_ReqFalse;
    UWORD   ic_Reserved2;
    UBYTE   ic_VDragModes[2][2];
};

/*********************************************************************************************/

static LONG stopchunks[] =
{
    ID_PREF, ID_ICTL
};

/*********************************************************************************************/

static void SetIControlPrefs(struct FileIControlPrefs *prefs)
{
    struct IIControlPrefs i;

    D(bug("[IcontrolPrefs_Handler: VDragModes: 0x%04X\n", prefs->ic_VDragModes[0][1]));

    #define GETBYTE(x) i.ic_ ## x = prefs->ic_ ## x
    #define GETWORD(x) i.ic_ ## x = ((prefs->ic_ ## x[0] << 8) + prefs->ic_ ## x[1])
    #define GETLONG(x) i.ic_ ## x = ((prefs->ic_ ## x[0] << 24) + \
    	    	    	    	     (prefs->ic_ ## x[1] << 16) + \
				     (prefs->ic_ ## x[2] << 8) + \
				      prefs->ic_ ## x[3])
    
    GETWORD(TimeOut);
    GETWORD(MetaDrag);
    GETLONG(Flags);
    GETBYTE(WBtoFront);
    GETBYTE(FrontToBack);
    GETBYTE(ReqTrue);
    GETBYTE(ReqFalse);
    GETWORD(VDragModes[0]);

    SetIPrefs(&i, sizeof(i), IPREFS_TYPE_ICONTROL);
}

/*********************************************************************************************/

void IControlPrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;
    
    D(bug("In IPrefs:IControlPrefs_Handler\n"));
    
    if ((iff = CreateIFF(filename, stopchunks, 1)))
    {
	while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
	{
	    struct FileIControlPrefs *icontrolprefs;

	    icontrolprefs = LoadChunk(iff, 0, MEMF_ANY);
	    if (icontrolprefs) {
    	    	D(bug("IControlPrefs_Handler: ID_ICTL chunk succesfully loaded.\n"));
    	    	SetIControlPrefs(icontrolprefs);
		FreeVec(icontrolprefs);		
	    }

	} /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */
	    
   	KillIFF(iff);
	
    } /* if ((iff = CreateIFF(filename))) */
    
    
    D(bug("In IPrefs:IControlPrefs_Handler. Done.\n", filename));
}

/*********************************************************************************************/
