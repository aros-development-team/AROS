/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function CloseWorkBench()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(LONG, CloseWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 13, Intuition)

/*  FUNCTION
	Attempt to close the Workbench screen:
	- Check for open application windows. return FALSE if there are any
	- Clean up all special buffers
	- Close the Workbench screen
	- Make the Workbench program mostly inactive
	  (disk activity will still be monitored)
	- Return TRUE

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenWorkBench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#if 0
    if( /* there are open app-windows */ )
    {
	return FALSE;
    }
    /* Clean up special buffers */

    /* Close the Workbench screen */
    CloseScreen( wbscreen );

    /* Make Workbanech task inactive */

    return TRUE;

#else

#warning TODO: Write intuition/CloseWorkBench()
    aros_print_not_implemented ("CloseWorkBench");

    return FALSE;

#endif

    AROS_LIBFUNC_EXIT
} /* CloseWorkBench */
