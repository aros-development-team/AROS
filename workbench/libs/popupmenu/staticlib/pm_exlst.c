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

	struct PM_IDLst *PM_ExLst(

/*  SYNOPSIS */
	IPTR id, 
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
    AROS_SLOWSTACKTAGS_PRE(id)

    retval = (IPTR)PM_ExLstA(AROS_SLOWSTACKTAGS_ARG(id));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_ExLst */
