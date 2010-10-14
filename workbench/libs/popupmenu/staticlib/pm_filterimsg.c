/*
    Copyright © 2010, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <utility/tagitem.h>
#include <proto/alib.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/popupmenu.h>
extern struct PopupMenuBase * PopupMenuBase;

	APTR PM_FilterIMsg(

/*  SYNOPSIS */
	struct Window *w,
	struct PopupMenu *pm,
	struct IntuiMessage *im,
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
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, APTR)

    retval = PM_FilterIMsgA(w, pm, im, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_FilterIMsg */
