/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Function for speedtest
    Lang: english
*/

#include <exec/types.h>

#include "gfxhidd_intern.h"
#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************

    NAME */

        AROS_LH3(VOID, HIDDF_Graphics_TestSpeed,

/*  SYNOPSIS */
        AROS_LHA(ULONG             , val1   , D2),
        AROS_LHA(ULONG             , val2   , D3),
        AROS_LHA(ULONG             , val3   , D4),

/*  LOCATION */
        struct Library *, GfxHiddBase, 5, GfxHidd)

/*  FUNCTION
        Just a function for the function vs. method call
        speedtest.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        02-04-98    drieling create
***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("Speedtest\n"));
    D(bug("val1: %i\n", val1));
    D(bug("val2: %i\n", val2));
    D(bug("val3: %i\n", val3));

    AROS_LIBFUNC_EXIT

} /* HIDDF_Graphics_TestSpeed */
