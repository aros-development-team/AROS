/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

    AROS_LH1(void, rtFreeRequest,

/*  SYNOPSIS */

	AROS_LHA(APTR, req, A1),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 6, ReqTools)

/*  FUNCTION
	Free requester structure previously allocated by rtAllocRequestA().
	This will also free all buffers associated with the requester, so
	there is no need to call rtFreeReqBuffer() first.
   
    INPUTS
	req - pointer to requester (may be NULL).

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	rtAllocRequestA()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeRequest(req); /* in filereqalloc.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtFreeRequest */
