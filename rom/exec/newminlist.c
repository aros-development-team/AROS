/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize a list
    Lang: english
*/

#include <exec/lists.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1I(void, NewMinList,

/*  SYNOPSIS */
	AROS_LHA(struct MinList *, list, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 138, Exec)

/*  FUNCTION
	Initialize a list. After that, you can use functions like
	AddHead(), AddTail() and Insert() on the list.

    INPUTS
	list - the list to be initialized

    RESULT
	None.

    NOTES

    EXAMPLE
	See below.

    BUGS

    SEE ALSO
	NEWLIST() macro, libamiga/NewList()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * The stupidest thing ever to waste an LVO with...
     * But some m68k applications (PFS3) already use it if exec.library version >= 45.
     * I wonder what's the win...
     */
    NEWLIST(list);

    AROS_LIBFUNC_EXIT
}
