/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include "general.h"
#include "reqtools_intern.h"
#include "rtfuncs.h"

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

	struct ReqToolsBase *, ReqToolsBase, 22, ReqTools)

/*  FUNCTION
	Evenly spread a number of objects over a certain length.
	Primary use is for arrangement of gadgets in a window.

    INPUTS
	posarray - pointer to array to be filled with positions.
	sizearray - pointer to array of sizes.
	totalsize - total size of all objects (sum of all values in
	    sizearray).
	min - first position to use.
	max - last position, first _NOT_ to use.
	num - number of objects (size of posarray and sizearray).

    RESULT
	none

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
	none known

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    RTFuncs_rtSpread(posarray, sizearray, totalsize, min, max, num);

    AROS_LIBFUNC_EXIT
    
} /* rtSpread */


