/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

static LONG stopchunks[] =
{
    ID_PREF, ID_ICTL
};

/*********************************************************************************************/

static void SetIControlPrefs(struct FileIControlPrefs *prefs)
{
    struct IIControlPrefs i;
    
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
	    struct ContextNode   *cn;
	    struct FileIControlPrefs icontrolprefs;

	    cn = CurrentChunk(iff);

   	    D(bug("IControlPrefs_Handler: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
		  cn->cn_Type >> 24,
		  cn->cn_Type >> 16,
		  cn->cn_Type >> 8,
		  cn->cn_Type,
		  cn->cn_ID >> 24,
		  cn->cn_ID >> 16,
		  cn->cn_ID >> 8,
		  cn->cn_ID));

	    if ((cn->cn_ID == ID_ICTL) && (cn->cn_Size == sizeof(icontrolprefs)))
	    {
    	    	D(bug("IControlPrefs_Handler: ID_ICTL chunk with correct size found.\n"));

		if (ReadChunkBytes(iff, &icontrolprefs, sizeof(icontrolprefs)) == sizeof(icontrolprefs))
		{
    	    	    SetIControlPrefs(&icontrolprefs);
		    
 		} /* if (ReadChunkBytes(iff, &icontrolprefs, sizeof(icontrolprefs)) == sizeof(icontrolprefs)) */
		
	    } /* if ((cn->cn_ID == ID_INPT) && (cn->cn_Size == sizeof(icontrolprefs))) */

	} /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */
	    
   	KillIFF(iff);
	
    } /* if ((iff = CreateIFF(filename))) */
    
    
    D(bug("In IPrefs:IControlPrefs_Handler. Done.\n", filename));
}

/*********************************************************************************************/
