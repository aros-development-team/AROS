/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include "aros_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/* Kickstart ROM location offsets */
#define LOC_COOKIE	0x00
#define LOC_ADDRESS	0x04
#define LOC_MAJORV	0x0c
#define LOC_MINORV	0x0e
#define LOC_ROMSIZE	0x14		/* offset from end of ROM! */
#define ROM_END		0x1000000

/*****************************************************************************

    NAME */
#include <proto/aros.h>
#include <aros/inquire.h>

	AROS_LH1(IPTR, ArosInquire,

/*  SYNOPSIS */

	AROS_LHA(ULONG, query, D0),

/*  LOCATION */

	struct ArosBase *, ArosBase, 5, Aros)

/*  FUNCTION
	This function is used to query system characteristics not easily
	queried with another function.

    INPUTS
	query -- which type of inquiry should be done

    RESULT
	Value depending on the type of query being done. See <aros/inquire.h>
	for details on this.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    IPTR ret = 0;

    D(bug("ArosInquire $%lx\n", query));

    switch(query)
    {
#if (AROS_FLAVOUR == AROS_FLAVOUR_NATIVE)
	/*
	    Only support these tags if we are on the native machine. On other
	    machines this call will return 0 for these tags.
	*/

	case AI_KickstartBase:
	    ret = (ROM_END - ArosInquire(AI_KickstartSize));
	    break;

	case AI_KickstartSize:
	    ret = *(ULONG *)(ROM_END - LOC_ROMSIZE);
	    break;

	case AI_KickstartVersion:
	    ret = (IPTR)(UWORD)*(UWORD *)(ArosInquire(AI_KickstartBase) + LOC_MAJORV);
	    break;

	case AI_KickstartRevision:
	    ret = (IPTR)(UWORD)*(UWORD *)(ArosInquire(AI_KickstartBase) + LOC_MINORV);
	    break;
#endif

	case AI_ArosVersion:
	    /*
		aros.library version masquerades as AROS version. This means
		that all aros modules must have the same major version number.
	    */
	    ret = (IPTR)(ULONG)LIBVERSION;
	    break;

	case AI_ArosReleaseMajor:
	    /* Update this whenever a new AROS is released */
	    ret = (IPTR)(ULONG)1;
	    break;

	case AI_ArosReleaseMinor:
	    /* Update this whenever a new AROS is released */
	    ret = (IPTR)(ULONG)11;
	    break;
    }

    return ret;
} /* ArosInquireTagList */
