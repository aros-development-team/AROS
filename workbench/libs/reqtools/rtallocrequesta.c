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

    AROS_LH2(APTR, rtAllocRequestA,

/*  SYNOPSIS */

	AROS_LHA(ULONG, type, D0),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 5, ReqTools)

/*  FUNCTION
	Allocates a requester structure for you in a future compatible
	manner. This is the only way to properly allocate a rtFileRequester,
	rtFontRequester, rtReqInfo or rtScreenModeRequester structure. The
	structure will be initialized for you.

	Use rtFreeRequest() to free the requester structure when you no
	longer need it.
   
    INPUTS
	type    - type of structure to allocate, currently RT_REQINFO,
	    RT_FILEREQ, RT_FONTREQ or RT_SCREENMODEREQ.
	taglist - pointer to array of tags (currently always NULL).

    TAGS
	no tags defined yet
	
    RESULT
	req - pointer to the requester allocated or NULL if no memory.

    NOTES

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	rtFreeRequest()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AllocRequestA(type, taglist); /* in filereqalloc.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtAllocRequestA */
