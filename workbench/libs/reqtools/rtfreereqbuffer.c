
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

    AROS_LH1(void, rtFreeReqBuffer,

/*  SYNOPSIS */

	AROS_LHA(APTR, req, A1),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 7, ReqTools)

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

    FreeReqBuffer(req); /* in filereqalloc.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtFreeReqBuffer */
