
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

    AROS_LH3(APTR, rtReqHandlerA,

/*  SYNOPSIS */

	AROS_LHA(struct rtHandlerInfo *, handlerinfo, A1),
	AROS_LHA(ULONG, sigs, D0),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct Library *, RTBase, 18, ReqTools)

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

    return  ((APTR (*)(struct rtHandlerInfo *, ULONG, struct TagItem *))handlerinfo->private1)(handlerinfo, sigs, taglist);
    
    AROS_LIBFUNC_EXIT
    
} /* rtReqHandlerA */
