/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define AROS_TAGRETURNTYPE ULONG
#include <utility/tagitem.h>
#include <libraries/reqtools.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	IPTR rtReqHandler (

/*  SYNOPSIS */
	struct rtHandlerInfo *handlerinfo,
	ULONG sigs,
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

    retval = rtReqHandlerA(handlerinfo, sigs, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtReqHandler */
