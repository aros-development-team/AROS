/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
 
    Free a copy of monitors list
*/

#include <proto/exec.h>

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */

#include <proto/intuition.h>

        AROS_LH1(void, FreeMonitorList,

/*  SYNOPSIS */
        AROS_LHA(Object **, list, A1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 162, Intuition)

/*  FUNCTION
	Frees an array of monitor class objects obtained using
	GetMonitorList().

    INPUTS
	list - a pointer to the list to free.

    RESULT
	None.

    NOTES
	This function is compatible with MorphOS v2.

    EXAMPLE

    BUGS

    SEE ALSO
	GetMonitorList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    FreeVec(list);
    
    AROS_LIBFUNC_EXIT
}
