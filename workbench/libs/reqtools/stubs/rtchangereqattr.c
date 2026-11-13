/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

#define AROS_TAGRETURNTYPE LONG
#include <utility/tagitem.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	LONG rtChangeReqAttr (

/*  SYNOPSIS */
	APTR req,
	Tag tag1, 
	...)

/*  FUNCTION
        Change the attributes of an existing requester.

    INPUTS
        req  - Requester whose attributes are to be changed.
        tag1 - First attribute tag; terminate the sequence with TAG_DONE.

    RESULT
        Return value from rtChangeReqAttrA(). It is meaningful for tags
        that explicitly define a result, including RTFI_AddEntry.

    NOTES
        Attribute applicability, ordering constraints, and tag-specific
        results are defined by rtChangeReqAttrA().

    SEE ALSO
        rtChangeReqAttrA()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtChangeReqAttrA(req, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtChangeReqAttr */
