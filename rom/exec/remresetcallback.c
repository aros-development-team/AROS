/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove reset callback from the internal list
    Lang: english
*/

#include <exec/interrupts.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1I(void, RemResetCallback,

/*  SYNOPSIS */
	AROS_LHA(struct Interrupt *, interrupt, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 168, Exec)

/*  FUNCTION
	Remove reset handler previously installed using AddResetCallBack()

    INPUTS
	interrupt - A pointer to an Interrupt structure

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    Remove(&interrupt->is_Node);

    AROS_LIBFUNC_EXIT
}
