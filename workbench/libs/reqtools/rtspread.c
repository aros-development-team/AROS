
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH6(VOID, rtSpread,

/*  SYNOPSIS */

	AROS_LHA(ULONG *, posarray , A0),
	AROS_LHA(ULONG *, sizearray, A1),
	AROS_LHA(ULONG  , totalsize, D0),
	AROS_LHA(ULONG  , min      , D1),
	AROS_LHA(ULONG  , max      , D2),
	AROS_LHA(ULONG  , num      , D3),

/*  LOCATION */

	struct Library *, RTBase, 22, ReqTools)

/*  FUNCTION

    Evenly spread a number of objects over a certain length.
    Primary use is for arrangement of gadgets in a window.

    INPUTS

    window  --  pointer to the window to be unlocked


    RESULT


    NOTES

    This function is for the advanced ReqTools user.

    EXAMPLE

    'sizearray' holds following values: 4, 6, 4, 2 and 8,
    'totalsize' is 24 (= 4 + 6 + 4 + 2 + 8),
    'min' is 3, 'max' is 43,
    and finally, 'num' is 5.
    
    After calling rtSpread() 'posarray' would hold the following
    values: 3, 11, 19, 26 and 31.
    
    My attempt at a visual representation:
    
        |                                            |
        |  |                                      |  |
        |  OOOO    OOOOOO    OOOO    OO    OOOOOOOO  |
        |  |                                      |  |
        |         1    1    2    2    3    3    4    4
        0----5----0----5----0----5----0----5----0----5

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG gadpos = min << 16;
    ULONG gadgap;
    UWORD i;

    gadgap = (max - min - totalsize) / (num - 1); 

    posarray[0] = min;

    for(i = 1; i < num - 1; i++)
    {
	gadpos += (sizearray[i] << 16) + gadgap;
	posarray[i] = gadpos >> 16;
    }

    posarray[num - 1] = max - sizearray[i];

    AROS_LIBFUNC_EXIT
} /* rtSpread */


