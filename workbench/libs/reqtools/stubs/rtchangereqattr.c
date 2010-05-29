/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    rtChangeReqAttrA(req, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtChangeReqAttr */
