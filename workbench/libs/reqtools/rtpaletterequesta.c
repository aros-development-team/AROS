
/*
    (C) 1999 - 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>

#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH3(APTR, rtPaletteRequestA,

/*  SYNOPSIS */

	AROS_LHA(char *, title, A2),
	AROS_LHA(struct rtReqInfo *, reqinfo, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct Library *, RTBase, 17, ReqTools)

/*  FUNCTION
   
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return PaletteRequestA(title, reqinfo, taglist); /* in palettereq.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtPaletteRequestA */
