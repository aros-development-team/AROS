
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

    AROS_LH2(LONG, rtChangeReqAttrA,

/*  SYNOPSIS */

	AROS_LHA(APTR, req, A1),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 8, ReqTools)

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

    return ChangeReqAttrA(req, taglist); /* in filereqalloc.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtChangeReqAttrA */
