/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions for readin .font files
    Lang: English.
*/

/****************************************************************************************/

#include <dos/dos.h>
#include <proto/exec.h>

#include "diskfont_intern.h"

/****************************************************************************************/

APTR AllocSegment(APTR prevsegment, ULONG segmentsize, ULONG memflags,
    	    	  struct DiskfontBase_intern *DiskfontBase)
{
    UBYTE *mem;
    
    if ((mem = AllocMem(segmentsize + sizeof(ULONG) + sizeof(BPTR), memflags)))
    {
    	*((ULONG *)mem)++ = segmentsize + sizeof(ULONG) + sizeof(BPTR);
	
	if (prevsegment)
	{
	    ((BPTR *)prevsegment)[-1] = MKBADDR(mem);
	}

	*((BPTR *)mem)++ = NULL;
		
    }
    
    return (APTR)mem;
}

/****************************************************************************************/
