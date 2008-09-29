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

    AROS_LH1(void, rtFreeReqBuffer,

/*  SYNOPSIS */

	AROS_LHA(APTR, req, A1),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 7, ReqTools)

/*  FUNCTION
	Frees the buffer associated with 'req'. In case of a file requester
	this function will deallocate the directory buffer, in case of a
	font requester the font list.

	It is safe to call this function for requesters that have no
	buffer, so you may call this for all requesters to free as much
	memory as possible.
   
    INPUTS
	req - pointer to requester.

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	rtFileRequestA(), rtFontRequestA()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeReqBuffer(req); /* in filereqalloc.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtFreeReqBuffer */
