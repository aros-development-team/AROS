/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function OpenWorkBench()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(ULONG, OpenWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 35, Intuition)

/*  FUNCTION
	Attempt to open the Workbench screen.

    INPUTS
	None.

    RESULT
	Tries to (re)open WorkBench screen. If successful return value
	is a pointer to the screen structure, which shouldn't be used,
	because other programs may close the WorkBench and make the
	pointer invalid.
	If this function fails the return value is NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseWorkBench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

struct Screen *wbscreen = NULL;

#if 0
struct IntuiMessage imsg;

    /* Do we have a running WorkBench app ? */
    if ( GetPrivIBase(IntuitionBase)->WorkBenchMP == NULL )
    {
	return NULL;
    }

    imsg.Class = WBENCHMESSAGE;
    imsg.Code = WBENCHOPEN;
    PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP, (struct IntuiMessage *)(&imsg) );
    /* Who opens the new Screen? If the WB-app does it, how do we get the
       new (struct Screen *wbscreen)? */

#else

#warning TODO: Write intuition/OpenWorkBench()
    aros_print_not_implemented ("OpenWorkBench");

#endif

    return (ULONG)wbscreen;

    AROS_LIBFUNC_EXIT
} /* OpenWorkBench */
