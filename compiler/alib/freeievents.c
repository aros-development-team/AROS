/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */

#include <libraries/commodities.h>
#include <proto/exec.h>
#include <devices/inputevent.h>

    VOID FreeIEvents(

/*  SYNOPSIS */

	struct InputEvent *ie)

/*  FUNCTION

    Frees a chain of input events allocated by InvertString() or InvertStringForwd().

    INPUTS

    ie    --  input event chain

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    InvertString(), InvertStringForwd()

    INTERNALS

    HISTORY

******************************************************************************/
{
    struct InputEvent *next;
    
    for(next = ie; next != NULL; ie = next)
    {
	next = ie->ie_NextEvent;

	if(ie->ie_Class == IECLASS_NEWPOINTERPOS &&
	   (ie->ie_SubClass == IESUBCLASS_TABLET ||
	    ie->ie_SubClass == IESUBCLASS_NEWTABLET ||
	    ie->ie_SubClass == IESUBCLASS_PIXEL))
	    FreeVec(ie->ie_EventAddress);
	
	FreeMem(ie, sizeof(struct InputEvent));
    }
} /* FreeIEvents */
