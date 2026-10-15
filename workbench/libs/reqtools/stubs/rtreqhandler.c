/*
    Copyright (C) 1995-2018, The AROS Development Team. All rights reserved.

    Desc:
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
        Continue processing a requester started with RT_ReqHandler.

    INPUTS
        handlerinfo - Handler state returned by the requester.
        sigs        - Signals received since the previous call.
        tag1        - First handler tag; terminate with TAG_DONE.

    RESULT
        CALL_HANDLER while additional handling is required; otherwise
        the requester's normal completion result.

    NOTES
        Check handlerinfo->DoNotWait and handlerinfo->WaitMask before
        waiting for signals. Continue calling this function while it
        returns CALL_HANDLER.

    SEE ALSO
        rtReqHandlerA(), rtEZRequestA()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtReqHandlerA(handlerinfo, sigs, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtReqHandler */
