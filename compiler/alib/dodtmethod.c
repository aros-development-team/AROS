/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif
#include "alib_intern.h"
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/alib.h>

extern struct Library * DataTypesBase;

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	IPTR DoDTMethod (

/*  SYNOPSIS */
	Object	    	 * obj,
	struct Window	 * win,
	struct Requester * req,
	ULONG		   methodId,
	...)

/*  FUNCTION
	Invokes a boopsi method on a datatype object with a GadgetInfo derived from
	the supplied window or requester parameter.

    INPUTS
	gad - The datatype object to work on
	win - The window which contains the gadget or the requester with
		the gadgets.
	req - If the gadget is in a requester, you must specify that one,
		too.
	methodId - The message to the gadget follows here.

    RESULT
	The result depends on the contents of the message sent to the
	gadget.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h
	25-10-96    calid   submitted the code

*****************************************************************************/
{
    AROS_SLOWSTACKMETHODS_PRE(methodId)
    retval = DoDTMethodA (obj
	, win
	, req
	, AROS_SLOWSTACKMETHODS_ARG(methodId)
    );
    AROS_SLOWSTACKMETHODS_POST
} /* DoDTMethod */
