/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

#define AROS_TAGRETURNTYPE APTR
#include <utility/tagitem.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	APTR rtAllocRequest (

/*  SYNOPSIS */
	ULONG type,
	Tag tag1, 
	...)

/*  FUNCTION
        Allocate and initialize a ReqTools requester structure.

    INPUTS
        type - Requester type (RT_REQINFO, RT_FILEREQ, RT_FONTREQ,
               or RT_SCREENMODEREQ).
        tag1 - First tag; terminate the tag/value sequence with TAG_DONE.

    RESULT
        Initialized requester structure, or NULL if allocation fails.

    NOTES
        No allocation tags are currently defined. Release the structure
        with rtFreeRequest().

    SEE ALSO
        rtAllocRequestA(), rtFreeRequest()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtAllocRequestA(type, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtAllocRequest */
