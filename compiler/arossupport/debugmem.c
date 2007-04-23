/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Show a dump of the memory list.
*/

#define DEBUG 1
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <aros/debug.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	void debugmem (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Print information about all memory lists.

    INPUTS
	None.

    RESULT
	None.

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
#ifndef __DONT_USE_DEBUGMEM__
    struct MemHeader *mh;
    struct MemChunk  *mc;

    Forbid();

    for
    (
        mh = (struct MemHeader *) GetHead(&SysBase->MemList); 
        mh != NULL; 
        mh = (struct MemHeader *) GetSucc(mh)
    )
    {
	bug("List %s: Attr=%08lX from 0x%p to 0x%p Free=%ld\n"
	    , mh->mh_Node.ln_Name
	    , mh->mh_Attributes
	    , mh->mh_Lower
	    , mh->mh_Upper
	    , mh->mh_Free
	);

	for (mc=mh->mh_First; mc; mc=mc->mc_Next)
	{
	    bug ("   Chunk %p Size %ld\n", mc, mc->mc_Bytes);
	}
    }

    Permit();
#endif
} /* debugmem */

