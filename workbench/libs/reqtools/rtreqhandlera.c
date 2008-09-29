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

#include "general.h"
#include "reqtools_intern.h"
#include "rtfuncs.h"

/*****************************************************************************

    NAME */

    AROS_LH3(ULONG, rtReqHandlerA,

/*  SYNOPSIS */

	AROS_LHA(struct rtHandlerInfo *, handlerinfo, A1),
	AROS_LHA(ULONG, sigs, D0),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 18, ReqTools)

/*  FUNCTION
	This function should be called if you used the RT_ReqHandler tag
	with a requester function.

	The requester you used the tag with will have returned immediately
	after its initialization and will have initialized a pointer to a
	rtHandlerInfo structure for you. You should now do the following:

	Check the DoNotWait field. If it is FALSE you have to wait for the
	signals in the WaitMask field (plus your own signals if you like).
	If any of the signals in WaitMask are received or DoNotWait was not
	FALSE you have to call rtReqHandlerA() and check its return value
	for one of the following values:

	CALL_HANDLER - Check DoNotWait again, Wait() if you have to and
	    call rtReqHandlerA() again. In other words, loop.
	everything else - normal return value, requester has finished. This
	    return value will be the same as if the requester had run
	    normally.

	You must pass the signals you received to rtReqHandlerA().

	NOTE: if you want to wait for your own signals do not do so if
	    DoNotWait is TRUE. Call rtReqHandlerA() and if you must know if
	    one of your signals arrived use SetSignal() to find this out.
	    If you are waiting for a message to arrive at a message port
	    you can simple call GetMsg() and check if it is non-null.
	    DoNotWait will naturally only be TRUE when it absolutely,
	    positively has to be. A multitasking machine as the Amiga
	    should use Wait() as much as possible.

	This is an example of a "requester loop":

	...
	struct rtHandlerInfo *hinfo;
	ULONG ret, mymask, sigs;

	...
	// calculate our mask 
	mymask = 1 << win->UserPort->mp_SigBit;

	// We use the RT_ReqHandler tag to cause the requester to return
	// after initializing.
	// Check the return value to see if this setup went ok.
	if( rtFontRequest( req, "Font", RT_ReqHandler, &hinfo, TAG_END )
	                                               == CALL_HANDLER )
	{
	    do
	    {
	        // Wait() if we can 
	        if( !hinfo->DoNotWait )
	        {
	            sigs = Wait( hinfo->WaitMask | mymask );
	        }
	        
	        // check our own message port 
	        while( msg = GetMsg( win->UserPort ) )
	        {
	            ...
	            // here we handle messages received at our windows
	            // IDCMP port
	            ...
	        }

	        // let the requester do its thing (remember to pass 'sigs') 
	        ret = rtReqHandler( hinfo, sigs, TAG_END );

	        // continue this loop as long as the requester is up
	    } while( ret == CALL_HANDLER )

	    // when we get here we know the requester has finished, 'ret'
	    // is the return code.
	    ...
        }
	else
	{
	    notify( "Error opening requester!" );
	}
	...
   
    INPUTS
	handlerinfo - pointer to handler info structure initialized by
	    using the RT_ReqHandler tag when calling a requester function.
	sigs - the signals received by previous wait, will be ignored if
	    handlerinfo->DoNotWait was TRUE.
	taglist - pointer to a TagItem array.

    TAGS
	RTRH_EndRequest - supplying this tag will end the requester. The
	    return code from rtReqHandlerA() will _not_ be CALL_HANDLER,
	    but the requester return code. If the tagdata of this tag is
	    REQ_CANCEL the requester will be canceled, if it is REQ_OK the
	    requester will be ok-ed. In case of an EZRequest tagdata should
	    be the return code of the requester (TRUE, FALSE or 2,3,4,...).

    RESULT
	ret - CALL_HANDLER if you have to call rtReqHandlerA() again, or
	    the normal return value from the requester.

    NOTES

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	rtEZRequestA(), (RT_ReqHandler explanation)

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return RTFuncs_rtReqHandlerA(handlerinfo, sigs, taglist);
        
    AROS_LIBFUNC_EXIT
    
} /* rtReqHandlerA */
